#include "UpnpPortMapping.h"
#include "TcpAsyncStream.h"
#include "NetAdaptersList.h"
#include "HttpHeader.h"

UpnpPortMapping::UpnpPortMapping(boost::asio::io_service& io_service) : io(io_service)
{
	state = std::make_shared<UpnpMappingState>();
}

UpnpPortMapping::~UpnpPortMapping()
{
	std::lock_guard<std::mutex> guard(state->stateMutex);

	state->active = false;
}

void UpnpPortMapping::mapActiveAdapters(uint16_t port, PortType type)
{
	auto adapters = NetAdapters::getActiveNetAdapters();

	for (auto& a : adapters)
	{
		if (!a.gateway.empty())
		{
			mapPort(a.gateway, a.clientIp, port, type, true);
		}
	}
}

void UpnpPortMapping::unmapMappedAdapters(uint16_t port, PortType type, bool waitForFinish)
{
	if(waitForFinish)
		waitForRequests();

	{
		for (auto& map : state->mappedPorts)
		{
			if((port == 0 || map.port == port) && map.type == type)
				unmapPort(map.gateway, map.port, map.type);
		}
	}

	if(waitForFinish)
		waitForRequests();
}

void UpnpPortMapping::unmapAllMappedAdapters(bool waitForFinish)
{
	unmapMappedAdapters(0, PortType::Udp, false);
	unmapMappedAdapters(0, PortType::Tcp, waitForFinish);
}

void UpnpPortMapping::mapPort(const std::string& gateway, const std::string& client, uint16_t port, PortType type, bool enable)
{
	{
		std::lock_guard<std::mutex> guard(state->stateMutex);

		for (auto it = state->pendingRequests.begin(); it != state->pendingRequests.end(); it++)
		{
			if ((*it)->getHostname() == gateway)
			{
				UpnpMappingState::TodoMapping todo;
				todo.mapping = { gateway, port, type };
				todo.client = client;
				todo.enable = enable;
				state->waitingMapping.push_back(todo);
				return;
			}
		}	
	}

	std::string portStr = std::to_string(port);
	std::string portType = type == PortType::Tcp ? "TCP" : "UDP";

	auto request = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<s:Envelope xmlns:s =\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<u:AddPortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\n"
		"<NewRemoteHost></NewRemoteHost>\n"
		"<NewExternalPort>" + portStr + "</NewExternalPort>\n"
		"<NewProtocol>" + portType + "</NewProtocol>\n"
		"<NewInternalPort>" + portStr + "</NewInternalPort>\n"
		"<NewInternalClient>" + client + "</NewInternalClient>\n"
		"<NewEnabled>" + std::string(enable ? "1" : "0") + "</NewEnabled>\n"
		"<NewPortMappingDescription>mtTorrent UPnP " + portStr + " " + portType + "</NewPortMappingDescription>\n"
		"<NewLeaseDuration>0</NewLeaseDuration>\n"
		"</u:AddPortMapping>\n"
		"</s:Body>\n"
		"</s:Envelope>\r\n";

	auto httpHeader = createUpnpHttpHeader(gateway + ":1900", request.length(), "urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping");

	auto stream = std::make_shared<TcpAsyncStream>(io);
	state->pendingRequests.push_back(stream);
	auto streamPtr = stream.get();
	auto upnpState = state;

	DataBuffer buffer;
	buffer.assign(httpHeader.begin(), httpHeader.end());
	buffer.insert(buffer.end(), request.begin(), request.end());

	stream->onConnectCallback = [streamPtr, buffer]()
	{
		streamPtr->write(buffer);
	};

	stream->onReceiveCallback = [streamPtr, upnpState, gateway, port, type]()
	{
		auto data = streamPtr->getReceivedData();
		auto header = HttpHeaderInfo::readFromBuffer(data);

		if (header.valid && data.size() >= (header.dataStart + header.dataSize))
		{
			streamPtr->consumeData(header.dataStart + header.dataSize);

			std::lock_guard<std::mutex> guard(upnpState->stateMutex);

			if (header.success)
			{
				upnpState->mappedPorts.push_back({ gateway, port, type });
			}
		}
	};

	stream->onCloseCallback = [streamPtr, upnpState, gateway, this](int code)
	{
		{
			std::lock_guard<std::mutex> guard(upnpState->stateMutex);

			for (auto it = upnpState->pendingRequests.begin(); it != upnpState->pendingRequests.end(); it++)
			{
				if ((*it).get() == streamPtr)
				{
					upnpState->pendingRequests.erase(it);
					break;
				}
			}
		}

		if (upnpState->active)
		{
			checkPendingMapping(gateway);
		}
	};

	stream->connect(gateway, 1900);
}

void UpnpPortMapping::unmapPort(const std::string& gateway, uint16_t port, PortType type)
{
	{
		std::lock_guard<std::mutex> guard(state->stateMutex);

		for (auto it = state->pendingRequests.begin(); it != state->pendingRequests.end(); it++)
		{
			if ((*it)->getHostname() == gateway)
			{
				UpnpMappingState::TodoMapping todo;
				todo.mapping = { gateway, port, type };
				todo.unmap = true;
				state->waitingMapping.push_back(todo);
				return;
			}
		}
	}

	std::string portStr = std::to_string(port);
	std::string portType = type == PortType::Tcp ? "TCP" : "UDP";

	auto request = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<u:DeletePortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">\n"
		"<NewRemoteHost></NewRemoteHost>\n"
		"<NewProtocol>" + portType + "</NewProtocol>\n"
		"<NewExternalPort>" + portStr + "</NewExternalPort>\n"
		"</u:DeletePortMapping>\n"
		"</s:Body>\n"
		"</s:Envelope>\r\n";

	auto httpHeader = createUpnpHttpHeader(gateway + ":1900", request.length(), "urn:schemas-upnp-org:service:WANIPConnection:1#DeletePortMapping");

	auto stream = std::make_shared<TcpAsyncStream>(io);
	auto streamPtr = stream.get();
	state->pendingRequests.push_back(stream);
	auto upnpState = state;

	DataBuffer buffer;
	buffer.assign(httpHeader.begin(), httpHeader.end());
	buffer.insert(buffer.end(), request.begin(), request.end());

	stream->onConnectCallback = [streamPtr, buffer]()
	{
		streamPtr->write(buffer);
	};

	stream->onReceiveCallback = [streamPtr, upnpState, gateway, port, type]()
	{
		auto data = streamPtr->getReceivedData();
		auto header = HttpHeaderInfo::readFromBuffer(data);

		if (header.valid && data.size() >= (header.dataStart + header.dataSize))
		{
			streamPtr->consumeData(header.dataStart + header.dataSize);

			std::lock_guard<std::mutex> guard(upnpState->stateMutex);

			if (header.success)
			{
				for (auto it = upnpState->mappedPorts.begin(); it != upnpState->mappedPorts.end(); it++)
				{
					if (it->gateway == gateway && it->port == port && it->type == type)
					{
						upnpState->mappedPorts.erase(it);
						break;
					}
				}
			}
		}
	};

	stream->onCloseCallback = [streamPtr, upnpState, gateway, this](int code)
	{
		{
			std::lock_guard<std::mutex> guard(upnpState->stateMutex);

			for (auto it = upnpState->pendingRequests.begin(); it != upnpState->pendingRequests.end(); it++)
			{
				if ((*it).get() == streamPtr)
				{
					upnpState->pendingRequests.erase(it);
					break;
				}
			}
		}

		if (upnpState->active)
		{
			checkPendingMapping(gateway);
		}
	};

	stream->connect(gateway, 1900);
}

void UpnpPortMapping::checkPendingMapping(const std::string& gateway)
{
	if (state->waitingMapping.empty())
		return;

	UpnpMappingState::TodoMapping todo;

	{
		std::lock_guard<std::mutex> guard(state->stateMutex);
		for (auto it = state->waitingMapping.begin(); it != state->waitingMapping.end(); it++)
		{
			if (it->mapping.gateway == gateway)
			{
				todo = *it;
				state->waitingMapping.erase(it);
				break;
			}
		}
	}

	if (!todo.mapping.gateway.empty())
	{
		if (todo.unmap)
			unmapPort(todo.mapping.gateway, todo.mapping.port, todo.mapping.type);
		else
			mapPort(todo.mapping.gateway, todo.client, todo.mapping.port, todo.mapping.type, todo.enable);
	}
}

void UpnpPortMapping::waitForRequests()
{
	int i = 0;
	while (i < 3000)	//3 sec timeout
	{
		if (!state->pendingRequests.empty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			i += 50;
		}
		else
			break;
	}
}

std::string UpnpPortMapping::createUpnpHttpHeader(const std::string& hostAddress, size_t contentLength, const std::string& soapAction)
{
	auto request = "POST /ipc HTTP/1.1\r\n"
		"SOAPAction: \"" + soapAction + "\"\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"User-Agent: mtTorrent (UPnP/1.0)\r\n"
		"Host: " + hostAddress + "\r\n"
		"Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: " + std::to_string(contentLength) + "\r\n"
		"\r\n";

	return request;
}

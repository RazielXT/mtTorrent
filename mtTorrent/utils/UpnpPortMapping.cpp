#include "UpnpPortMapping.h"
#include "TcpAsyncStream.h"
#include "NetAdaptersList.h"
#include "HttpHeader.h"
#include "Logging.h"

#define UPNP_LOG(x) WRITE_GLOBAL_LOG(Upnp, x)

const std::string upnpMappingServiceName = "urn:schemas-upnp-org:service:WANIPConnection:1";

UpnpPortMapping::UpnpPortMapping(asio::io_context& io_context) : io(io_context), discovery(io_context)
{
	state = std::make_shared<UpnpMappingState>();
}

UpnpPortMapping::~UpnpPortMapping()
{
	std::lock_guard<std::mutex> guard(state->mtx);

	state->active = false;
	discovery.stop();

	UPNP_LOG("stopping");
}

void UpnpPortMapping::mapActiveAdapters(uint16_t port, PortType type)
{
	std::lock_guard<std::mutex> guard(state->mtx);

	for (auto& m : mapping)
	{
		if (m.port == port && m.type == type)
			return;
	}

	mapping.push_back({ port, type });
	UPNP_LOG("Map " << type << " port " << port);

	if (!discovery.active())
		discovery.start([this](UpnpDiscovery::DeviceInfo& device)
		{
			std::lock_guard<std::mutex> guard(state->mtx);

			for (auto& d : foundDevices)
			{
				if (d.clientIp == device.clientIp && d.gateway == device.gateway && d.port == device.port)
				{
					return;
				}
			}

			UPNP_LOG("Found device " << device.name << " gateway " << device.gateway);
			foundDevices.push_back(device);

			refreshMapping();
		});


	refreshMapping();
}

void UpnpPortMapping::unmapMappedAdapters(uint16_t port, PortType type)
{
	std::lock_guard<std::mutex> guard(state->mtx);
	UPNP_LOG("Unmap " << type << " port " << port);

	for (auto it = mapping.begin(); it != mapping.end(); it++)
	{
		if (it->port == port && it->type == type)
		{
			mapping.erase(it);
			break;
		}
	}

	refreshMapping();
}

void UpnpPortMapping::unmapAllMappedAdapters(bool waitForFinish)
{
	UPNP_LOG("unmapAllMappedAdapters");

	discovery.stop();
	{
		std::lock_guard<std::mutex> guard(state->mtx);
		mapping.clear();
		refreshMapping();
	}

	if (waitForFinish)
	{
		std::unique_lock<std::mutex> lk(state->mtx);
		state->cv.wait_for(lk, std::chrono::seconds(3), [this] { return state->mappedPorts.empty(); });
	}
}

void UpnpPortMapping::mapPort(const std::string& gateway, uint16_t gatewayPort, const std::string& client, uint16_t port, PortType type, bool enable)
{
	UPNP_LOG("mapPort " << gateway << " port " << port << " type " << type);

	std::string portStr = std::to_string(port);
	std::string portType = type == PortType::Tcp ? "TCP" : "UDP";

	auto request = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<s:Envelope xmlns:s =\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<u:AddPortMapping xmlns:u=\"" + upnpMappingServiceName + "\">\n"
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

	auto httpHeader = createUpnpHttpHeader(gateway, std::to_string(gatewayPort), request.length(), upnpMappingServiceName + "#AddPortMapping");

	auto stream = std::make_shared<TcpAsyncStream>(io);
	state->pendingRequests.push_back(stream);
	auto streamPtr = stream.get();
	auto upnpState = state;

	DataBuffer buffer;
	buffer.assign(httpHeader.begin(), httpHeader.end());
	buffer.insert(buffer.end(), request.begin(), request.end());

	stream->onConnectCallback = [streamPtr, buffer, gateway]()
	{
		UPNP_LOG("Connected device mapping " << gateway);
		streamPtr->write(buffer);
	};

	stream->onReceiveCallback = [upnpState, gateway, gatewayPort, port, type](const BufferView& data) -> std::size_t
	{
		auto header = HttpHeaderInfo::read((const char*)data.data, data.size);

		if (header.valid && data.size >= (header.dataStart + header.dataSize))
		{
			std::lock_guard<std::mutex> guard(upnpState->mtx);

			if (header.success)
			{
				UPNP_LOG("Mapped device " << gateway << " port " << port << " type " << type);
				upnpState->mappedPorts.push_back({ gateway, gatewayPort, port, type });
			}

			return header.dataStart + header.dataSize;
		}
		return 0;
	};

	stream->onCloseCallback = [streamPtr, upnpState, gateway, this](int code)
	{
		std::lock_guard<std::mutex> guard(upnpState->mtx);

		UPNP_LOG("Disconnected device mapping " << gateway);
		for (auto it = upnpState->pendingRequests.begin(); it != upnpState->pendingRequests.end(); it++)
		{
			if ((*it).get() == streamPtr)
			{
				upnpState->pendingRequests.erase(it);
				break;
			}
		}

		if (upnpState->active)
			refreshMapping();
	};

	stream->connect(gateway, gatewayPort);
}

void UpnpPortMapping::unmapPort(const std::string& gateway, uint16_t gatewayPort, uint16_t port, PortType type)
{
	UPNP_LOG("unmapPort " << gateway << " port " << port << " type " << type);

	std::string portStr = std::to_string(port);
	std::string portType = type == PortType::Tcp ? "TCP" : "UDP";

	auto request = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
		"<s:Body>\n"
		"<u:DeletePortMapping xmlns:u=\"" + upnpMappingServiceName + "\">\n"
		"<NewRemoteHost></NewRemoteHost>\n"
		"<NewProtocol>" + portType + "</NewProtocol>\n"
		"<NewExternalPort>" + portStr + "</NewExternalPort>\n"
		"</u:DeletePortMapping>\n"
		"</s:Body>\n"
		"</s:Envelope>\r\n";

	auto httpHeader = createUpnpHttpHeader(gateway, std::to_string(gatewayPort), request.length(), upnpMappingServiceName + "#DeletePortMapping");

	auto stream = std::make_shared<TcpAsyncStream>(io);
	auto streamPtr = stream.get();
	state->pendingRequests.push_back(stream);
	auto upnpState = state;

	DataBuffer buffer;
	buffer.assign(httpHeader.begin(), httpHeader.end());
	buffer.insert(buffer.end(), request.begin(), request.end());

	stream->onConnectCallback = [streamPtr, buffer, gateway]()
	{
		UPNP_LOG("Connected device unmapping " << gateway);
		streamPtr->write(buffer);
	};

	stream->onReceiveCallback = [streamPtr, upnpState, gateway, port, type](const BufferView& data) -> std::size_t
	{
		auto header = HttpHeaderInfo::read((const char*)data.data, data.size);

		if (header.valid && data.size >= (header.dataStart + header.dataSize))
		{
			std::lock_guard<std::mutex> guard(upnpState->mtx);

			if (header.success)
			{
				for (auto it = upnpState->mappedPorts.begin(); it != upnpState->mappedPorts.end(); it++)
				{
					if (it->gateway == gateway && it->port == port && it->type == type)
					{
						UPNP_LOG("Unmapped device " << gateway << " port " << port << " type " << type);
						upnpState->mappedPorts.erase(it);
						upnpState->cv.notify_one();
						break;
					}
				}
			}

			return header.dataStart + header.dataSize;
		}
		return 0;
	};

	stream->onCloseCallback = [streamPtr, upnpState, gateway, this](int code)
	{
		std::lock_guard<std::mutex> guard(upnpState->mtx);

		UPNP_LOG("Disconnected device unmapping " << gateway);
		for (auto it = upnpState->pendingRequests.begin(); it != upnpState->pendingRequests.end(); it++)
		{
			if ((*it).get() == streamPtr)
			{
				upnpState->pendingRequests.erase(it);
				break;
			}
		}

		if (upnpState->active)
			refreshMapping();
	};

	stream->connect(gateway, gatewayPort);
}

std::string UpnpPortMapping::getReadableInfo() const
{
	std::string info;

	std::lock_guard<std::mutex> guard(state->mtx);

	for (const auto& d : foundDevices)
	{
		info += d.name + " (" + d.clientIp + ":" + std::to_string(d.port) + ")";

		for (auto& map : state->mappedPorts)
		{
			if (d.gateway == map.gateway)
			{
				info += (map.type == PortType::Tcp ? ", Tcp " : ", Udp ") + std::to_string(map.port);
			}
		}

		for (auto& request : state->pendingRequests)
		{
			if (request->getHostname() == d.gateway)
			{
				info += ", Pending..";
			}
		}
	}

	return info;
}

void UpnpPortMapping::refreshMapping()
{
	UPNP_LOG("refreshMapping");

	for (auto& device : foundDevices)
	{
		for (auto& r : state->pendingRequests)
		{
			if (r->getHostname() == device.gateway)
				return;
		}

		//map new
		for (auto& m : mapping)
		{
			bool found = false;
			for (auto& map : state->mappedPorts)
			{
				if (map.gateway == device.gateway && map.port == m.port && map.type == m.type)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				mapPort(device.gateway, device.port, device.clientIp, m.port, m.type, true);
				break;
			}
		}

		//unmap missing
		for (auto& map : state->mappedPorts)
		{
			bool found = false;
			for (auto& m : mapping)
			{
				if (map.gateway == device.gateway && map.port == m.port && map.type == m.type)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				unmapPort(device.gateway, device.port, map.port, map.type);
				break;
			}
		}
	}
}

std::string UpnpPortMapping::getMappingServiceControlUrl(const std::string& gateway)
{
	for (auto& d : foundDevices)
	{
		if (d.gateway == gateway)
			return d.services[upnpMappingServiceName];
	}

	return "";
}

std::string UpnpPortMapping::createUpnpHttpHeader(const std::string& hostAddress, const std::string& port, std::size_t contentLength, const std::string& soapAction)
{
	std::string controlUrl = getMappingServiceControlUrl(hostAddress);
	if (controlUrl.empty())
		controlUrl = "/ipc";//"/ctl/IPConn";

	auto request = "POST " + controlUrl + " HTTP/1.1\r\n"
		"SOAPAction: \"" + soapAction + "\"\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"User-Agent: mtTorrent (UPnP/1.0)\r\n"
		"Host: " + hostAddress + ":" + port + "\r\n"
		"Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: " + std::to_string(contentLength) + "\r\n"
		"\r\n";

	return request;
}

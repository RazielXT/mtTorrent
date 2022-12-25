#include "UpnpDiscovery.h"
#include "NetAdaptersList.h"
#include "UdpAsyncComm.h"
#include "XmlParser.h"

UpnpDiscovery::UpnpDiscovery(asio::io_service& io_service) : io(io_service)
{

}

struct UrlParser
{
	std::string protocol;
	std::string address;
	uint16_t port;
	std::string path;

	UrlParser(const std::string& url)
	{
		size_t addrStart = 0;

		auto protocolEnd = url.find_first_of("://");

		if (protocolEnd != std::string::npos)
		{
			addrStart = protocolEnd + 3;
			protocol = url.substr(0, protocolEnd);
		}

		auto portStart = url.find_first_of(':', addrStart);
		auto pathStart = url.find_first_of('/', addrStart);

		if (portStart != std::string::npos || pathStart != std::string::npos)
			address = url.substr(addrStart, std::min(portStart, pathStart) - addrStart);
		else
			address = url.substr(addrStart);

		if (portStart != std::string::npos)
		{
			portStart++;
			std::string portStr;

			if (pathStart != std::string::npos)
				portStr = url.substr(portStart, pathStart - portStart);
			else
				portStr = url.substr(portStart);

			port = (uint16_t)strtoul(portStr.data(), nullptr, 10);
		}

		if (pathStart != std::string::npos)
		{
			path = url.substr(pathStart);
		}
	}
};

void UpnpDiscovery::start(std::function<void(DeviceInfo&)> onNewDeviceCb)
{
	if (search.active())
		return;

	search.start([this](HttpHeaderInfo* response) 
		{
			for (auto& p : response->headerParameters)
			{
				if (p.first == "LOCATION")
				{
					UrlParser url(p.second);

					if (!url.address.empty() && url.port)
					{
						auto adapters = NetAdapters::getActiveNetAdapters();

						for (auto& a : adapters)
						{
							std::lock_guard<std::mutex> guard(mtx);

							if (a.gateway == url.address)
								upnpLocationQueue.push_back({ a , url.port, url.path });
						}
					}
				}
			}

			queryNext();
		});

	onNewDevice = onNewDeviceCb;
}

void UpnpDiscovery::stop()
{
	search.stop();

	std::lock_guard<std::mutex> guard(mtx);

	upnpLocationQueue.clear();
	onNewDevice = nullptr;

	if (stream)
	{
		stream->close();
		stream.reset();
	}
}

void UpnpDiscovery::queryNext()
{
	std::lock_guard<std::mutex> guard(mtx);

	if (upnpLocationQueue.empty())
		return;

	auto upnpLocation = upnpLocationQueue.back();
	upnpLocationQueue.pop_back();

	std::string rootInfoRequest = "GET " + upnpLocation.rootXmlLocation + " HTTP/1.1\r\n"
		"User-Agent: mtTorrent\r\n"
		"Connection: close\r\n"
		"Host: " + upnpLocation.adapter.gateway + ":" + std::to_string(upnpLocation.port) + "\r\n"
		"Accept: text/xml\r\n\r\n";

	DataBuffer buffer;
	buffer.assign(rootInfoRequest.begin(), rootInfoRequest.end());

	stream = std::make_shared<TcpAsyncStream>(io);
	auto streamPtr = stream.get();

	stream->onConnectCallback = [streamPtr, buffer]()
	{
		streamPtr->write(buffer);
	};

	DeviceInfo info;
	info.gateway = upnpLocation.adapter.gateway;
	info.clientIp = upnpLocation.adapter.clientIp;
	info.port = upnpLocation.port;

	stream->onReceiveCallback = [streamPtr, this, info](const BufferView& data) -> std::size_t
	{
		auto header = HttpHeaderInfo::read((const char*)data.data, data.size);

		if (header.valid && data.size >= (header.dataStart + header.dataSize))
		{
			onRootXmlReceive((const char*)data.data + header.dataStart, header.dataSize, info);
			return header.dataStart + header.dataSize;
		}
		return 0;
	};

	stream->onCloseCallback = [streamPtr, this](int code)
	{
		queryNext();
	};

	stream->connect(upnpLocation.adapter.gateway, upnpLocation.port);
}

static void parseDevicesNode(const mtt::xml::Element* node, std::string& name, std::map<std::string, std::string>& services)
{
	auto deviceNode = node->firstNode("device");
	while (deviceNode)
	{
		if (name.empty())
			name = deviceNode->value("friendlyName");

		if (auto servicesNode = deviceNode->firstNode("serviceList"))
		{
			auto sNode = servicesNode->firstNode("service");
			while (sNode)
			{
				std::string type = std::string(sNode->value("serviceType"));
				std::string controlUrl = std::string(sNode->value("controlURL"));

				if (!type.empty() && !controlUrl.empty())
					services[type] = controlUrl;

				sNode = sNode->nextSibling("service");
			}
		}

		if (auto devicesNode = deviceNode->firstNode("deviceList"))
		{
			parseDevicesNode(devicesNode, name, services);
		}

		deviceNode = deviceNode->nextSibling("device");
	}
}

void UpnpDiscovery::onRootXmlReceive(const char* xml, uint32_t size, DeviceInfo info)
{
	mtt::xml::Document doc;
	if (doc.parse(xml, size))
	{
		parseDevicesNode(doc.getRoot(), info.name, info.services);
		onNewDevice(info);
	}
}

void UpnpDiscovery::SsdpSearch::start(std::function<void(HttpHeaderInfo*)> onResponse)
{
	std::string requestStr = "M-SEARCH * HTTP/1.1\r\n"
		"ST: upnp:rootdevice\r\n"
		"MX: 3\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"HOST: 239.255.255.250:1900\r\n\r\n";

	DataBuffer buffer;
	buffer.assign(requestStr.begin(), requestStr.end());

	request = UdpAsyncComm::Get()->sendMessage(buffer, "239.255.255.250", "1900", [this, onResponse](UdpRequest, DataBuffer* data)
		{
			std::lock_guard<std::mutex> guard(mtx);

			if (data)
			{
				HttpHeaderInfo info = HttpHeaderInfo::readFromBuffer(*data);

				if (info.success)
					onResponse(&info);

				if (info.valid)
				{
					request.reset();
					return true;
				}
			}
			else
				request.reset();

			return false;
		}
	, false, 2, true);
}

void UpnpDiscovery::SsdpSearch::stop()
{
	std::lock_guard<std::mutex> guard(mtx);

	if (request)
	{
		UdpAsyncComm::Get()->removeCallback(request);
		request.reset();
	}
}

bool UpnpDiscovery::SsdpSearch::active()
{
	return request != nullptr;
}

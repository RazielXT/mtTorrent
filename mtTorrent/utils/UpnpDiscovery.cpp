#include "UpnpDiscovery.h"
#include "NetAdaptersList.h"
#include "UdpAsyncComm.h"
#include "Xml.h"

UpnpDiscovery::UpnpDiscovery(asio::io_context& io_context) : io(io_context)
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

	search.start([this](HttpHeaderInfo* response, const NetAdapters::NetAdapter& adapter)
		{
			for (auto& p : response->headerParameters)
			{
				if (p.first == "LOCATION")
				{
					UrlParser url(p.second);

					if (!url.address.empty() && url.port)
					{
						std::lock_guard<std::mutex> guard(mtx);

						if (adapter.gateway == url.address)
							upnpLocationQueue.push_back({ adapter , url.port, url.path });
					}
				}
			}

			queryNext();
		}, io);

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

bool UpnpDiscovery::active() const
{
	return search.active();
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

static void parseDevicesNode(const mtt::XmlParser::Element* node, std::string& name, std::map<std::string, std::string>& services)
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
	mtt::XmlParser::Document doc;
	if (doc.parse(xml, size))
	{
		parseDevicesNode(doc.getRoot(), info.name, info.services);
		onNewDevice(info);
	}
}

void UpnpDiscovery::SsdpSearch::start(std::function<void(HttpHeaderInfo*, const NetAdapters::NetAdapter&)> onResponse, asio::io_context& io)
{
	auto adapters = NetAdapters::getActiveNetAdapters();
	for (auto& a : adapters)
	{
		if (!a.gateway.empty() && a.clientIp != "0.0.0.0")
		{
			NetAdapters::NetAdapter adapter = a;

			auto request = std::make_shared<UdpAsyncWriter>(io);
			request->setAddress(Addr({ 239,255,255,250 }, 1900));
			request->setBindAddress(Addr::asioFromString(adapter.clientIp.c_str()));
			request->setBroadcast(true);
			request->onResponse = [this, onResponse, adapter](UdpRequest, const BufferView& data)
			{
				std::lock_guard<std::mutex> guard(mtx);

				if (data.size)
				{
					HttpHeaderInfo info = HttpHeaderInfo::readFromBuffer(data);

					if (info.success)
						onResponse(&info, adapter);
				}
			};

			const std::string_view requestStr = "M-SEARCH * HTTP/1.1\r\n"
				"ST: upnp:rootdevice\r\n"
				"MX: 3\r\n"
				"MAN: \"ssdp:discover\"\r\n"
				"HOST: 239.255.255.250:1900\r\n\r\n";

			DataBuffer buffer;
			buffer.assign(requestStr.begin(), requestStr.end());

			request->write(buffer);
			requests.push_back(request);
		}
	}
}

void UpnpDiscovery::SsdpSearch::stop()
{
	std::lock_guard<std::mutex> guard(mtx);

	for (auto& r : requests)
	{
		r->close();
	}

	requests.clear();
}

bool UpnpDiscovery::SsdpSearch::active() const
{
	return !requests.empty();
}

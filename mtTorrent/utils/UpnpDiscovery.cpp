#include "UpnpDiscovery.h"
#include "NetAdaptersList.h"
#include "HttpHeader.h"

#define PUGIXML_HEADER_ONLY
#include "pugixml.hpp"

UpnpDiscovery::UpnpDiscovery(asio::io_service& io_service) : io(io_service)
{

}

void UpnpDiscovery::start(std::function<void()> onFinishCb)
{
	{
		std::lock_guard<std::mutex> guard(mtx);

		if (!adaptersQueue.empty())
			return;
	}

	auto adapters = NetAdapters::getActiveNetAdapters();

	for (auto& a : adapters)
	{
		if (!a.gateway.empty())
			adaptersQueue.push_back(a);
	}

	onFinish = onFinishCb;
	queryNext();
}

void UpnpDiscovery::stop()
{
	std::lock_guard<std::mutex> guard(mtx);

	adaptersQueue.clear();
	onFinish = nullptr;

	if (stream)
	{
		stream->close();
		stream.reset();
	}
}

void UpnpDiscovery::queryNext()
{
	std::lock_guard<std::mutex> guard(mtx);

	if (adaptersQueue.empty())
	{
		if (onFinish)
			onFinish();

		return;
	}

	auto& adapter = adaptersQueue.back();

	std::string rootInfoRequest = "GET /rootDesc.xml HTTP/1.1\r\n"
		"User-Agent: mtTorrent\r\n"
		"Connection: close\r\n"
		"Host: " + adapter.gateway + ":5000\r\n"
		"Accept: text/xml\r\n\r\n";

	DataBuffer buffer;
	buffer.assign(rootInfoRequest.begin(), rootInfoRequest.end());

	stream = std::make_shared<TcpAsyncStream>(io);
	auto streamPtr = stream.get();

	stream->onConnectCallback = [streamPtr, buffer]()
	{
		streamPtr->write(buffer);
	};

	stream->onReceiveCallback = [streamPtr, this]()
	{
		auto data = streamPtr->getReceivedData();
		auto header = HttpHeaderInfo::readFromBuffer(data);

		if (header.valid && data.size() >= (header.dataStart + header.dataSize))
		{
			streamPtr->consumeData(header.dataStart + header.dataSize);
			onRootXmlReceive((const char*)data.data() + header.dataStart, header.dataSize);
		}
	};

	stream->onCloseCallback = [streamPtr, this](int code)
	{
		adaptersQueue.pop_back();
		queryNext();
	};

	DeviceInfo info;
	info.gateway = adapter.gateway;
	info.clientIp = adapter.clientIp;
	info.port = 5000;
	devices.push_back(info);

	stream->connect(adapter.gateway, 5000);
}

static void parseDevicesNode(pugi::xml_node node, std::string& name, std::map<std::string, std::string>& services)
{
	auto deviceNode = node.child("device");
	while (deviceNode)
	{
		if (name.empty())
			name = deviceNode.child("friendlyName").child_value();

		auto servicesNode = deviceNode.child("serviceList");
		if (servicesNode)
		{
			auto sNode = servicesNode.child("service");
			while (sNode)
			{
				std::string type = sNode.child("serviceType").child_value();
				std::string controlUrl = sNode.child("controlURL").child_value();

				if (!type.empty() && !controlUrl.empty())
					services[type] = controlUrl;

				sNode = sNode.next_sibling("service");
			}
		}
	
		auto devicesNode = deviceNode.child("deviceList");
		if (devicesNode)
		{
			parseDevicesNode(devicesNode, name, services);
		}

		deviceNode = deviceNode.next_sibling("device");
	}
}

void UpnpDiscovery::onRootXmlReceive(const char* xml, uint32_t size)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(xml, size);

	if (result.status == pugi::status_ok)
	{
		auto& info = devices.back();

		for(auto child : doc.children())
			parseDevicesNode(child, info.name, info.services);
	}
}


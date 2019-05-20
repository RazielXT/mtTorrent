#pragma once
#include "asio/io_service.hpp"
#include <functional>
#include <map>
#include "NetAdaptersList.h"
#include "TcpAsyncStream.h"
#include "UdpAsyncWriter.h"
#include "HttpHeader.h"

class UpnpDiscovery
{
public:

	UpnpDiscovery(asio::io_service& io_service);

	struct DeviceInfo
	{
		std::string name;
		std::string gateway;
		std::string clientIp;
		uint16_t port;

		std::map<std::string, std::string> services;
	};

	void start(std::function<void(DeviceInfo&)> onNewDevice);
	void stop();

private:

	void queryNext();
	void onRootXmlReceive(const char* xml, uint32_t size, DeviceInfo info);

	std::mutex mtx;
	std::function<void(DeviceInfo&)> onNewDevice;

	struct UpnpLocation
	{
		NetAdapters::NetAdapter adapter;
		uint16_t port;
		std::string rootXmlLocation;
	};
	std::vector<UpnpLocation> upnpLocationQueue;

	std::shared_ptr<TcpAsyncStream> stream;
	asio::io_service& io;

	struct SsdpSearch
	{
		void start(std::function<void(HttpHeaderInfo*)> onResponse);
		void stop();
		bool active();

		std::mutex mtx;
		UdpRequest request;
	}
	search;
};
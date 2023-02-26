#pragma once

#include "asio/io_context.hpp"
#include "NetAdaptersList.h"
#include "TcpAsyncStream.h"
#include "UdpAsyncWriter.h"
#include "HttpHeader.h"
#include <functional>
#include <map>

class UpnpDiscovery
{
public:

	UpnpDiscovery(asio::io_context& io_context);

	struct DeviceInfo
	{
		std::string name;
		std::string gateway;
		std::string clientIp;
		uint16_t port = 0;

		std::map<std::string, std::string> services;
	};

	void start(std::function<void(DeviceInfo&)> onNewDevice);
	void stop();

	bool active() const;

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
	asio::io_context& io;

	struct SsdpSearch
	{
		void start(std::function<void(HttpHeaderInfo*, const NetAdapters::NetAdapter& adapter)> onResponse, asio::io_context& io);
		void stop();
		bool active() const;

		mutable std::mutex mtx;
		std::vector<UdpRequest> requests;
	}
	search;
};
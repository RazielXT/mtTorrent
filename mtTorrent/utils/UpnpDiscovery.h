#pragma once
#include "asio/io_service.hpp"
#include <functional>
#include <map>
#include "NetAdaptersList.h"
#include "TcpAsyncStream.h"

class UpnpDiscovery
{
public:

	UpnpDiscovery(asio::io_service& io_service);

	void start(std::function<void()> onFinish);
	void stop();

	struct DeviceInfo
	{
		std::string name;
		std::string gateway;
		std::string clientIp;
		uint16_t port;

		std::map<std::string, std::string> services;
	};

	std::vector<DeviceInfo> devices;

private:

	void queryNext();
	void onRootXmlReceive(const char* xml, uint32_t size);

	std::mutex mtx;
	std::function<void()> onFinish;
	std::vector<NetAdapters::NetAdapter> adaptersQueue;

	std::shared_ptr<TcpAsyncStream> stream;
	asio::io_service& io;
};
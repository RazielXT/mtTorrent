#pragma once

#include "UpnpDiscovery.h"
#include "utils/ServiceThreadpool.h"
#include <condition_variable>

class TcpAsyncStream;

class UpnpPortMapping
{
public:

	UpnpPortMapping(asio::io_context& io_context);
	~UpnpPortMapping();

	enum PortType { Tcp, Udp };
	void mapActiveAdapters(uint16_t port, PortType type);
	void unmapMappedAdapters(uint16_t port, PortType type);

	void unmapAllMappedAdapters(bool waitForFinish = true);

	std::string getReadableInfo() const;

private:

	void mapPort(const std::string& gateway, uint16_t gatewayPort, const std::string& client, uint16_t port, PortType type, bool enable);
	void unmapPort(const std::string& gateway, uint16_t gatewayPort, uint16_t port, PortType type);

	void refreshMapping();

	std::string createUpnpHttpHeader(const std::string& hostAddress, const std::string& port, std::size_t contentLength, const std::string& soapAction);
	std::string getMappingServiceControlUrl(const std::string& gateway);

	asio::io_context& io;

	struct UpnpMappingState
	{
		bool active = true;
		mutable std::mutex mtx;
		std::condition_variable cv;

		struct MappedPort
		{
			std::string gateway;
			uint16_t gatewayPort;
			uint16_t port;
			PortType type;
		};
		std::vector<MappedPort> mappedPorts;

		std::vector<std::shared_ptr<TcpAsyncStream>> pendingRequests;
	};
	std::shared_ptr<UpnpMappingState> state;

	struct Mapping
	{
		uint16_t port;
		PortType type;
	};
	std::vector<Mapping> mapping;

	UpnpDiscovery discovery;
	std::vector<UpnpDiscovery::DeviceInfo> foundDevices;
};
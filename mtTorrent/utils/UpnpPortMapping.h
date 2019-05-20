#pragma once
#include "utils/ServiceThreadpool.h"
#include "UpnpDiscovery.h"

class TcpAsyncStream;

class UpnpPortMapping
{
public:

	UpnpPortMapping(asio::io_service& io_service);
	~UpnpPortMapping();

	enum class PortType { Tcp, Udp };
	void mapActiveAdapters(uint16_t port, PortType type);
	void unmapMappedAdapters(uint16_t port, PortType type, bool waitForFinish = true);
	void unmapAllMappedAdapters(bool waitForFinish = true);

	void mapPort(const std::string& gateway, uint16_t gatewayPort, const std::string& client, uint16_t port, PortType type, bool enable);
	void unmapPort(const std::string& gateway, uint16_t gatewayPort, uint16_t port, PortType type);

private:

	std::string getMappingServiceControlUrl(const std::string& gateway);

	void checkPendingMapping(const std::string& gateway);

	void waitForRequests();

	std::string createUpnpHttpHeader(const std::string& hostAddress, const std::string& port, size_t contentLength, const std::string& soapAction);

	asio::io_service& io;

	struct UpnpMappingState
	{
		bool active = true;
		std::mutex stateMutex;

		struct MappedPort
		{
			std::string gateway;
			uint16_t gatewayPort;
			uint16_t port;
			PortType type;
		};
		std::vector<MappedPort> mappedPorts;

		struct TodoMapping
		{
			MappedPort mapping;
			bool enable;
			std::string client;
			bool unmap = false;
		};
		std::vector<TodoMapping> waitingMapping;
		std::vector<std::shared_ptr<TcpAsyncStream>> pendingRequests;
	};
	std::shared_ptr<UpnpMappingState> state;

	std::string upnpMappingServiceName;

	bool discoveryStarted = false;
	bool discoveryFinished = false;
	std::mutex discoveryMutex;
	std::vector<std::pair<uint16_t, PortType>> activeMappingPending;
	UpnpDiscovery discovery;
};
#pragma once

#include "utils\Network.h"
#include "UdpAsyncReceiver.h"
#include "ServiceThreadpool.h"

using UdpResponseCallback = std::function<bool(UdpRequest, DataBuffer*)>;

class UdpAsyncComm;
using UdpCommPtr = std::shared_ptr<UdpAsyncComm>;

class UdpAsyncComm
{
public:

	static UdpCommPtr Get();
	static void Deinit();

	void setBindPort(uint16_t port);

	void listen(UdpPacketCallback received);
	void removeListeners();

	UdpRequest create(std::string& host, std::string& port);
	UdpRequest sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpResponseCallback response, bool ipv6 = false);
	UdpRequest sendMessage(DataBuffer& data, Addr& addr, UdpResponseCallback response);
	void sendMessage(DataBuffer& data, UdpRequest target, UdpResponseCallback response, uint32_t timeout = 1);
	void sendMessage(DataBuffer& data, udp::endpoint& endpoint);

	void removeCallback(UdpRequest target);

private:

	struct ResponseRetryInfo
	{
		UdpRequest client;
		uint8_t retries = 0;
		UdpResponseCallback onResponse;
		std::shared_ptr<asio::steady_timer> timeoutTimer;
		uint32_t defaultTimeout = 1;
		void reset();
	};

	std::mutex respondingMutex;
	std::mutex responsesMutex;
	std::vector<std::shared_ptr<ResponseRetryInfo>> pendingResponses;
	void addPendingResponse(DataBuffer& data, UdpRequest target, UdpResponseCallback response, uint32_t timeout = 1);
	UdpRequest findPendingConnection(UdpRequest);

	void checkTimeout(UdpRequest, const asio::error_code& error);
	void onUdpReceive(udp::endpoint&, DataBuffer&);
	void onUdpClose(UdpRequest);
	UdpPacketCallback onUnhandledReceive;

	void startListening();
	std::shared_ptr<UdpAsyncReceiver> listener;
	uint16_t bindPort = 0;

	ServiceThreadpool pool;
};

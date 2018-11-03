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

	void setBindPort(uint16_t port);

	void listen(UdpPacketCallback received);

	UdpRequest create(std::string& host, std::string& port);
	UdpRequest sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpResponseCallback response, bool ipv6 = false);
	UdpRequest sendMessage(DataBuffer& data, Addr& addr, UdpResponseCallback response);
	void sendMessage(DataBuffer& data, UdpRequest target, UdpResponseCallback response);
	void sendMessage(DataBuffer& data, udp::endpoint& endpoint);

private:

	struct ResponseRetryInfo
	{
		UdpRequest client;
		uint8_t retries = 0;
		UdpResponseCallback onResponse;
		std::shared_ptr<boost::asio::deadline_timer> timeoutTimer;
		void reset();
	};

	std::mutex responsesMutex;
	std::vector<std::shared_ptr<ResponseRetryInfo>> pendingResponses;
	void addPendingResponse(DataBuffer& data, UdpRequest target, UdpResponseCallback response);
	UdpRequest findPendingConnection(UdpRequest);

	void checkTimeout(std::shared_ptr<ResponseRetryInfo>);
	void onUdpReceive(udp::endpoint&, DataBuffer&);
	void onUdpClose(UdpRequest);
	UdpPacketCallback onUnhandledReceive;

	void startListening();
	std::shared_ptr<UdpAsyncReceiver> listener;
	uint16_t bindPort = 0;

	ServiceThreadpool pool;
};

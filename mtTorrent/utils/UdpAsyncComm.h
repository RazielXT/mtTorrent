#pragma once

#include "utils\Network.h"
#include "UdpAsyncReceiver.h"

class UdpAsyncComm
{
public:

	UdpAsyncComm(boost::asio::io_service& io, uint16_t port);

	void listen(UdpResponseCallback received);

	UdpRequest create(std::string& host, std::string& port);
	UdpRequest sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpResponseCallback response);
	UdpRequest sendMessage(DataBuffer& data, Addr& addr, UdpResponseCallback response);
	void sendMessage(DataBuffer& data, UdpRequest target, UdpResponseCallback response);

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
	void onUdpReceive(UdpRequest, DataBuffer&);
	void onUdpClose(UdpRequest);
	UdpResponseCallback onUnhandledReceive;

	std::shared_ptr<UdpAsyncReceiver> listener;
	uint16_t bindPort;

	boost::asio::io_service& io;
};

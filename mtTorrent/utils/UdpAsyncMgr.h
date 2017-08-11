#pragma once

#include "utils\Network.h"
#include "UdpAsyncServer.h"

class UdpAsyncMgr
{
public:

	UdpAsyncMgr(boost::asio::io_service& io);

	void listen(uint16_t port, UdpConnectionCallback received);

	UdpConnection create(std::string& host, std::string& port);
	UdpConnection sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpConnectionCallback response);
	UdpConnection sendMessage(DataBuffer& data, Addr& addr, UdpConnectionCallback response);
	void sendMessage(DataBuffer& data, UdpConnection target, UdpConnectionCallback response);

private:

	struct ResponseRetryInfo
	{
		UdpConnection client;
		DataBuffer writeData;
		uint8_t tryCount = 0;
		UdpConnectionCallback onResponse;

		std::shared_ptr<boost::asio::deadline_timer> timeoutTimer;
		void reset();
	};

	std::mutex responsesMutex;
	std::vector<std::shared_ptr<ResponseRetryInfo>> pendingResponses;
	void addPendingResponse(DataBuffer& data, UdpConnection target, UdpConnectionCallback response);
	UdpConnection findPendingConnection(UdpConnection);

	void checkTimeout(std::shared_ptr<ResponseRetryInfo>);
	void onUdpReceive(UdpConnection, DataBuffer&);
	void onUdpClose(UdpConnection);
	UdpConnectionCallback onReceive;

	std::shared_ptr<UdpAsyncServer> listener;

	boost::asio::io_service& io;
};

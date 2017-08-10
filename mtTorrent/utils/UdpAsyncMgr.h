#pragma once

#include "Network.h"
#include "UdpAsyncClient.h"

using UdpConnectionCallback = std::function<bool(UdpConnection, DataBuffer*)>;

class UdpAsyncMgr
{
public:

	UdpAsyncMgr(boost::asio::io_service& io);

	void setImplicitPort(uint16_t port);
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
		uint8_t writeRetries = 0;
		UdpConnectionCallback onResponse;

		std::shared_ptr<boost::asio::deadline_timer> timeoutTimer;
		void reset();
	};

	std::mutex responsesMutex;
	std::vector<std::shared_ptr<ResponseRetryInfo>> pendingResponses;
	void addPendingResponse(DataBuffer& data, UdpConnection target, UdpConnectionCallback response);
	UdpConnection findPendingConnection(UdpConnection);

	uint16_t implicitPort = 0;

	void checkTimeout(std::shared_ptr<ResponseRetryInfo>);
	void onUdpReceive(UdpConnection, DataBuffer&);
	void onUdpClose(UdpConnection);
	UdpConnectionCallback onReceive;

	boost::asio::io_service& io;
};

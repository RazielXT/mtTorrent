#pragma once

#include "UdpAsyncWriter.h"

using UdpPacketCallback = std::function<void(udp::endpoint&, std::vector<DataBuffer*>&)>;

class UdpAsyncReceiver : public std::enable_shared_from_this<UdpAsyncReceiver>
{
public:

	UdpAsyncReceiver(asio::io_service& io_service, uint16_t port, bool ipv6);

	void listen();
	void stop();

	UdpPacketCallback receiveCallback;

private:

	void handle_receive(const std::error_code& error);

	bool active = false;
	udp::socket socket_;

	void readSocket();

	static constexpr size_t MaxReadIterations = 30;
	struct
	{
		udp::endpoint endpoint;
		DataBuffer data[MaxReadIterations];
		std::vector<DataBuffer*> dataVec;
	}
	tmp;

	static constexpr size_t ListenBufferSize = 2048;
};

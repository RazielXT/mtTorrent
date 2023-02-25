#pragma once

#include "UdpAsyncWriter.h"

using UdpPacketCallback = std::function<void(udp::endpoint&, std::vector<BufferView>&)>;

class UdpAsyncReceiver : public std::enable_shared_from_this<UdpAsyncReceiver>
{
public:

	UdpAsyncReceiver(asio::io_context& io_context, uint16_t port, bool ipv6);

	void listen();
	void stop();

	UdpPacketCallback receiveCallback;

private:

	void handle_receive(const std::error_code& error);

	bool active = false;
	udp::socket socket_;

	void readSocket();

	const std::size_t MinBufferSize = 10 * 1024;
	const std::size_t MaxBufferSize = 50 * 1024;
	const std::size_t MinBufferReadSize = 2048;
	struct
	{
		udp::endpoint endpoint;
		DataBuffer buffer;
		std::vector<BufferView> packets;
	}
	tmp;
	void flushPackets();
};

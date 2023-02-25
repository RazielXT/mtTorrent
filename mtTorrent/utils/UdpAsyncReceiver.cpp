#include "UdpAsyncReceiver.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_GLOBAL_LOG(UdpListener, x)

UdpAsyncReceiver::UdpAsyncReceiver(asio::io_context& io_context, uint16_t port, bool ipv6) : socket_(io_context)
{
	auto myEndpoint = udp::endpoint(ipv6 ? udp::v6() : udp::v4(), port);
	std::error_code ec;
	socket_.open(myEndpoint.protocol(), ec);
	socket_.set_option(asio::socket_base::reuse_address(true), ec);
	socket_.bind(myEndpoint, ec);
	socket_.non_blocking(true, ec);
}

void UdpAsyncReceiver::listen()
{
	active = true;
	tmp.buffer.resize(MinBufferSize);
	socket_.async_receive(asio::null_buffers(), std::bind(&UdpAsyncReceiver::handle_receive, shared_from_this(), std::placeholders::_1));
}

void UdpAsyncReceiver::stop()
{
	active = false;
	socket_.cancel();
}

void UdpAsyncReceiver::handle_receive(const std::error_code& error)
{
	if (!active)
		return;

	if (!error)
		readSocket();

	socket_.async_receive(asio::null_buffers(), std::bind(&UdpAsyncReceiver::handle_receive, shared_from_this(), std::placeholders::_1));
}

void UdpAsyncReceiver::readSocket()
{
	std::error_code ec;
	auto available = std::min(MaxBufferSize, socket_.available(ec));
	UDP_LOG("readSocket available " << available);

	if (available > tmp.buffer.size())
	{
		tmp.buffer.resize(available);
	}

	udp::endpoint lastEndpoint;
	size_t bufferPos = 0;

	while (tmp.buffer.size() - bufferPos >= MinBufferReadSize)
	{
		size_t transferred = socket_.receive_from(asio::buffer(tmp.buffer.data() + bufferPos, tmp.buffer.size() - bufferPos), tmp.endpoint, 0, ec);

		if (ec || !transferred)
		{
			if (ec == asio::error::interrupted)
				continue;

			if (ec != asio::error::would_block)
				UDP_LOG("receive_from error: " << ec.message());

			break;
		}

		UDP_LOG("transferred " << transferred);

		if (lastEndpoint != tmp.endpoint)
			flushPackets();
		else
			UDP_LOG("append");

		tmp.packets.emplace_back(tmp.buffer.data() + bufferPos, transferred);

		bufferPos += transferred;
		lastEndpoint = tmp.endpoint;
	}

	flushPackets();
	UDP_LOG("total " << bufferPos);
}

void UdpAsyncReceiver::flushPackets()
{
	if (!tmp.packets.empty())
	{
		UDP_LOG(tmp.endpoint.address().to_string() << " sent " << tmp.packets.size() << " buffers");

		if (receiveCallback)
			receiveCallback(tmp.endpoint, tmp.packets);

		tmp.packets.clear();
	}
}

#include "UdpAsyncReceiver.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG(LogTypeUdpListener, x)

UdpAsyncReceiver::UdpAsyncReceiver(asio::io_service& io_service, uint16_t port, bool ipv6) : socket_(io_service)
{
	auto myEndpoint = udp::endpoint(ipv6 ? udp::v6() : udp::v4(), port);
	std::error_code ec;
	socket_.open(myEndpoint.protocol(), ec);
	socket_.set_option(asio::socket_base::reuse_address(true), ec);
	socket_.bind(myEndpoint, ec);
}

void UdpAsyncReceiver::listen()
{
	active = true;
	buffer.resize(BufferSize);
	socket_.async_receive_from(asio::buffer(buffer.data(), buffer.size()), remote_endpoint_,	
		std::bind(&UdpAsyncReceiver::handle_receive, this, std::placeholders::_1, std::placeholders::_2));
}

void UdpAsyncReceiver::stop()
{
	active = false;
	socket_.cancel();
}

void UdpAsyncReceiver::handle_receive(const std::error_code& error, std::size_t bytes_transferred)
{
	UDP_LOG(remote_endpoint_.address().to_string() << " sent bytes " << bytes_transferred);

	//if (!bytes_transferred && error.value() != 10054)
	//	return;

	if (bytes_transferred && receiveCallback)
	{
		buffer.resize(bytes_transferred);
		receiveCallback(remote_endpoint_, buffer);
	}

	listen();
}


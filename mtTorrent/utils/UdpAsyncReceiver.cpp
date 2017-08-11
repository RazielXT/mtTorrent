#include "UdpAsyncReceiver.h"

UdpAsyncReceiver::UdpAsyncReceiver(boost::asio::io_service& io_service, uint16_t port, bool ipv6) : socket_(io_service)
{
	auto myEndpoint = udp::endpoint(ipv6 ? udp::v6() : udp::v4(), port);
	socket_.open(myEndpoint.protocol());
	socket_.set_option(boost::asio::socket_base::reuse_address(true));
	socket_.bind(myEndpoint);
}

void UdpAsyncReceiver::listen()
{
	active = true;
	buffer.resize(BufferSize);
	socket_.async_receive_from(boost::asio::buffer(buffer.data(), buffer.size()), remote_endpoint_,	
		std::bind(&UdpAsyncReceiver::handle_receive, this, std::placeholders::_1, std::placeholders::_2));
}

void UdpAsyncReceiver::stop()
{
	active = false;
	socket_.cancel();
}

void UdpAsyncReceiver::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (receiveCallback)
	{
		UdpRequest connection = std::make_shared<UdpAsyncWriter>(socket_.get_io_service());
		connection->setAddress(remote_endpoint_);
		buffer.resize(bytes_transferred);

		receiveCallback(connection, buffer);
	}

	listen();
}


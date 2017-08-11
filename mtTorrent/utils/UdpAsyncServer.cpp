#include "UdpAsyncServer.h"

UdpAsyncServer::UdpAsyncServer(boost::asio::io_service& io_service, uint16_t port, bool ipv6) : socket_(io_service)
{
	auto myEndpoint = udp::endpoint(ipv6 ? udp::v6() : udp::v4(), port);
	socket_.open(myEndpoint.protocol());
	socket_.set_option(boost::asio::socket_base::reuse_address(true));
	socket_.bind(myEndpoint);
}

void UdpAsyncServer::listen()
{
	active = true;
	buffer.resize(BufferSize);
	socket_.async_receive_from(boost::asio::buffer(buffer.data(), buffer.size()), remote_endpoint_,	
		std::bind(&UdpAsyncServer::handle_receive, this, std::placeholders::_1, std::placeholders::_2));
}

void UdpAsyncServer::stop()
{
	active = false;
	socket_.cancel();
}

void UdpAsyncServer::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (receiveCallback)
	{
		UdpConnection connection = std::make_shared<UdpAsyncClient>(socket_.get_io_service());
		connection->setAddress(remote_endpoint_);
		buffer.resize(bytes_transferred);

		receiveCallback(connection, buffer);
	}

	listen();
}


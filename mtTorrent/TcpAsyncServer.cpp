#include "TcpAsyncServer.h"


TcpAsyncServer::TcpAsyncServer(boost::asio::io_service& io_service) : acceptor_(io_service)
{
}

void TcpAsyncServer::listen(uint16_t port, bool ipv6)
{
	acceptor_.bind(boost::asio::ip::tcp::endpoint(ipv6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), port));
	startListening();
}

void TcpAsyncServer::startListening()
{
	auto connection = std::make_shared<TcpAsyncStream>(acceptor_.get_io_service());

	acceptor_.async_accept(connection->socket, std::bind(&TcpAsyncServer::handle_accept, this, connection, std::placeholders::_1));
}

void TcpAsyncServer::handle_accept(std::shared_ptr<TcpAsyncStream> connection, const boost::system::error_code& error)
{
	if (!error)
	{
		connection->setAsConnected();

		if (acceptCallback)
			acceptCallback(connection);

		startListening();
	}
}

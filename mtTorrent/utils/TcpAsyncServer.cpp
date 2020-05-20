#include "TcpAsyncServer.h"
#include "Logging.h"

#define TCP_LOG(x) WRITE_LOG(LogTypeTcp, x)

TcpAsyncServer::TcpAsyncServer(asio::io_service& io_service, uint16_t port, bool ipv6) : endpoint(ipv6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), port), acceptor_(io_service, endpoint)
{
}

void TcpAsyncServer::listen()
{
	startListening();
}

void TcpAsyncServer::stop()
{
	acceptor_.close();
}

void TcpAsyncServer::startListening()
{
	auto connection = std::make_shared<TcpAsyncLimitedStream>(acceptor_.get_io_service());

	acceptor_.async_accept(connection->socket, endpoint, std::bind(&TcpAsyncServer::handle_accept, this, connection, std::placeholders::_1));
}

void TcpAsyncServer::handle_accept(std::shared_ptr<TcpAsyncLimitedStream> connection, const std::error_code& error)
{
	if (!error && acceptor_.is_open())
	{
		connection->setAsConnected();

		if (acceptCallback)
			acceptCallback(connection);

		startListening();
	}
	else
		TCP_LOG("accept: " << error.message())
}

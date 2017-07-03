#include "TcpAsyncStream.h"

class TcpAsyncServer
{
public:

	TcpAsyncServer(boost::asio::io_service& io_service) : acceptor_(io_service)
	{
	}

	void listen(uint16_t port, bool ipv6)
	{
		acceptor_.bind(boost::asio::ip::tcp::endpoint(ipv6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), port));
		startListening();
	}

	void setOnAcceptCallback(std::function<void(std::shared_ptr<TcpAsyncStream>)> func)
	{
		acceptCallback = func;
	}

private:

	void startListening()
	{
		auto connection = std::make_shared<TcpAsyncStream>(acceptor_.get_io_service());

		acceptor_.async_accept(connection->socket,
			std::bind(&TcpAsyncServer::handle_accept, this, connection, std::placeholders::_1));
	}

	void handle_accept(std::shared_ptr<TcpAsyncStream> connection, const boost::system::error_code& error)
	{
		if (!error)
		{
			if(acceptCallback)
				acceptCallback(connection);

			startListening();
		}
	}

	std::function<void(std::shared_ptr<TcpAsyncStream>)> acceptCallback;
	boost::asio::ip::tcp::acceptor acceptor_;
};

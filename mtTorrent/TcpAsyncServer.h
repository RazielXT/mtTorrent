#include "TcpAsyncStream.h"

class TcpAsyncServer
{
public:

	TcpAsyncServer(boost::asio::io_service& io_service, uint16_t port, bool ipv6);

	void listen();

	std::function<void(std::shared_ptr<TcpAsyncStream>)> acceptCallback;

private:

	void startListening();

	void handle_accept(std::shared_ptr<TcpAsyncStream> connection, const boost::system::error_code& error);
	
	tcp::endpoint endpoint;

	boost::asio::ip::tcp::acceptor acceptor_;

};

#include "TcpAsyncStream.h"

class TcpAsyncServer
{
public:

	TcpAsyncServer(asio::io_service& io_service, uint16_t port, bool ipv6);

	void listen();
	void stop();

	std::function<void(std::shared_ptr<TcpAsyncStream>)> acceptCallback;

private:

	void startListening();

	void handle_accept(std::shared_ptr<TcpAsyncStream> connection, const std::error_code& error);
	
	tcp::endpoint endpoint;

	asio::ip::tcp::acceptor acceptor_;

};

#include "TcpAsyncStream.h"

class TcpAsyncServer : public std::enable_shared_from_this<TcpAsyncServer>
{
public:

	TcpAsyncServer(asio::io_context& io_context, uint16_t port, bool ipv6);

	void listen();
	void stop();

	std::function<void(std::shared_ptr<TcpAsyncStream>)> acceptCallback;

private:

	void startListening();

	void handle_accept(std::shared_ptr<TcpAsyncStream> connection, const std::error_code& error);
	
	tcp::endpoint endpoint;

	asio::ip::tcp::acceptor acceptor_;
	asio::io_context& io;

	std::mutex mtx;
};

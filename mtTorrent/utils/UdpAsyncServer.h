#include "UdpAsyncClient.h"

using UdpConnectionCallback = std::function<bool(UdpConnection, DataBuffer*)>;

class UdpAsyncServer
{
public:

	UdpAsyncServer(boost::asio::io_service& io_service, uint16_t port, bool ipv6);

	void listen();
	void stop();

	UdpConnectionCallback receiveCallback;

private:

	void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

	bool active = false;
	udp::socket socket_;
	udp::endpoint remote_endpoint_;

	const size_t BufferSize = 2 * 1024;
	DataBuffer buffer;

};

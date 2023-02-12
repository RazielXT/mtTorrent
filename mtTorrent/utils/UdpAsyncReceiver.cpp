#include "UdpAsyncReceiver.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_GLOBAL_LOG(UdpListener, x)

UdpAsyncReceiver::UdpAsyncReceiver(asio::io_context& io_context, uint16_t port, bool ipv6) : socket_(io_context)
{
	auto myEndpoint = udp::endpoint(ipv6 ? udp::v6() : udp::v4(), port);
	std::error_code ec;
	socket_.open(myEndpoint.protocol(), ec);
	socket_.set_option(asio::socket_base::reuse_address(true), ec);
	socket_.bind(myEndpoint, ec);
	socket_.non_blocking(true, ec);
}

void UdpAsyncReceiver::listen()
{
	active = true;
	socket_.async_receive(asio::null_buffers(), std::bind(&UdpAsyncReceiver::handle_receive, shared_from_this(), std::placeholders::_1));
}

void UdpAsyncReceiver::stop()
{
	active = false;
	socket_.cancel();
}

void UdpAsyncReceiver::handle_receive(const std::error_code& error)
{
	if (!active)
		return;

	if (!error)
		readSocket();

	socket_.async_receive(asio::null_buffers(), std::bind(&UdpAsyncReceiver::handle_receive, shared_from_this(), std::placeholders::_1));
}

void UdpAsyncReceiver::readSocket()
{
	size_t bufferStart = 0;
	size_t buffer = 0;
	udp::endpoint lastEndpoint;
	tmp.dataVec.clear();

	for (size_t i = 0; i < MaxReadIterations; i++)
	{
		std::error_code ec;
		//auto av = socket_.available(ec);
		tmp.data[buffer].resize(ListenBufferSize);
		size_t transferred = socket_.receive_from(asio::buffer(tmp.data[buffer]), tmp.endpoint, 0, ec);

		if (ec)
		{
			if (ec == asio::error::interrupted)
				continue;

			if (ec != asio::error::would_block)
				UDP_LOG("receive_from error: " << ec.message());

			break;
		}

		if (transferred)
		{
			UDP_LOG("transferred " << transferred);

			tmp.data[buffer].resize(transferred);
			tmp.dataVec.push_back(&tmp.data[buffer]);

			if (buffer > 0 && lastEndpoint != tmp.endpoint)
			{
				if (receiveCallback)
					receiveCallback(tmp.endpoint, tmp.dataVec);

				bufferStart = buffer;
				tmp.dataVec.clear();
			}

			buffer++;
			lastEndpoint = tmp.endpoint;
		}
	}

	if (buffer && receiveCallback)
		receiveCallback(tmp.endpoint, tmp.dataVec);

	UDP_LOG(tmp.endpoint.address().to_string() << " sent " << buffer << " buffers");
}

#include "UdpAsyncWriter.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG(LogTypeUdp, x)

UdpAsyncWriter::UdpAsyncWriter(boost::asio::io_service& io) : io_service(io), socket(io)
{
}

UdpAsyncWriter::~UdpAsyncWriter()
{
}

void UdpAsyncWriter::setAddress(Addr& addr)
{
	target_endpoint = addr.toUdpEndpoint();

	state = Initialized;
}

void UdpAsyncWriter::setAddress(const std::string& hostname, const std::string& port)
{
	udp::resolver::query query(hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncWriter::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncWriter::setAddress(const std::string& hostname, const std::string& port, bool ipv6)
{
	udp::resolver::query query(ipv6 ? udp::v6() : udp::v4(), hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncWriter::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncWriter::setAddress(udp::endpoint& addr)
{
	target_endpoint = addr;

	state = Initialized;
}

void UdpAsyncWriter::setBindPort(uint16_t port)
{
	bindPort = port;
}

void UdpAsyncWriter::close()
{
	onCloseCallback = nullptr;

	io_service.post(std::bind(&UdpAsyncWriter::do_close, shared_from_this()));
}

void UdpAsyncWriter::write(const DataBuffer& data)
{
	io_service.post(std::bind(&UdpAsyncWriter::do_write, shared_from_this(), data));
}

void UdpAsyncWriter::write()
{
	io_service.post(std::bind(&UdpAsyncWriter::do_rewrite, this));
}

void UdpAsyncWriter::handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		target_endpoint = *iterator;
		state = Initialized;

		if (!messageBuffer.empty())
			send_message();
	}
	else
	{
		postFail("Resolve", error);
	}
}

void UdpAsyncWriter::postFail(std::string place, const boost::system::error_code& error)
{
	if(error)
		UDP_LOG(place << "-" << error.message())

	if(state == Connected)
		state = Initialized;

	if (onCloseCallback)
		onCloseCallback(shared_from_this());
}

void UdpAsyncWriter::handle_connect(const boost::system::error_code& error)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		state = Connected;
		send_message();
	}
	else
	{
		postFail("Connect", error);
	}
}

void UdpAsyncWriter::do_close()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	onCloseCallback = nullptr;

	if (state == Connected)
		state = Initialized;

	boost::system::error_code error;

	if (socket.is_open())
	{
		socket.shutdown(boost::asio::socket_base::shutdown_both);
		socket.close(error);
	}	
}

void UdpAsyncWriter::do_rewrite()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (state != Clear)
		send_message();
}

void UdpAsyncWriter::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	messageBuffer = data;

	if (state != Clear)
		send_message();
}

void UdpAsyncWriter::send_message()
{
	if (state == Initialized)
	{
		if (bindPort && !socket.is_open())
		{
			boost::system::error_code ec;

			socket.open(target_endpoint.protocol(), ec);
			socket.set_option(boost::asio::socket_base::reuse_address(true),ec);
			socket.bind(udp::endpoint(target_endpoint.protocol(), bindPort), ec);

			if (ec)
			{
				UDP_LOG("port bind error: " << ec.message());
			}
		}

		socket.async_connect(target_endpoint, std::bind(&UdpAsyncWriter::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if (state == Connected)
	{
		if (!messageBuffer.empty())
		{
			UDP_LOG("writing " << messageBuffer.size() << " bytes");

			socket.async_send_to(boost::asio::buffer(messageBuffer.data(), messageBuffer.size()), target_endpoint,
				std::bind(&UdpAsyncWriter::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}
}

void UdpAsyncWriter::handle_write(const boost::system::error_code& error, size_t sz)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (error)
	{
		postFail("Write", error);
	}
}


std::string UdpAsyncWriter::getName()
{
	return target_endpoint.address().to_string() + ":" + std::to_string(target_endpoint.port());
}

udp::endpoint& UdpAsyncWriter::getEndpoint()
{
	return target_endpoint;
}

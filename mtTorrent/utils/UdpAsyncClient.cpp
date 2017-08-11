#include "UdpAsyncClient.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG("UDP: " << getName() << " " << x)

UdpAsyncClient::UdpAsyncClient(boost::asio::io_service& io) : io_service(io), socket(io)
{
}

UdpAsyncClient::~UdpAsyncClient()
{
}

void UdpAsyncClient::setAddress(Addr& addr)
{
	target_endpoint = addr.ipv6 ?
		udp::endpoint(boost::asio::ip::address_v6(*reinterpret_cast<boost::asio::ip::address_v6::bytes_type*>(addr.addrBytes)), addr.port) :
		udp::endpoint(boost::asio::ip::address_v4(*reinterpret_cast<boost::asio::ip::address_v4::bytes_type*>(addr.addrBytes)), addr.port);

	state = Initialized;
}

void UdpAsyncClient::setAddress(const std::string& hostname, const std::string& port)
{
	udp::resolver::query query(hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncClient::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncClient::setAddress(const std::string& hostname, const std::string& port, bool ipv6)
{
	udp::resolver::query query(ipv6 ? udp::v6() : udp::v4(), hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncClient::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncClient::setAddress(udp::endpoint& addr)
{
	target_endpoint = addr;

	state = Initialized;
}

void UdpAsyncClient::setImplicitPort(uint16_t port)
{
	implicitPort = port;
}

void UdpAsyncClient::close()
{
	onCloseCallback = nullptr;

	io_service.post(std::bind(&UdpAsyncClient::do_close, shared_from_this()));
}

void UdpAsyncClient::write(const DataBuffer& data)
{
	io_service.post(std::bind(&UdpAsyncClient::do_write, this, data));
}

void UdpAsyncClient::write()
{
	io_service.post(std::bind(&UdpAsyncClient::do_rewrite, this));
}

void UdpAsyncClient::handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver)
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

void UdpAsyncClient::postFail(std::string place, const boost::system::error_code& error)
{
	if(error)
		UDP_LOG(place << "-" << error.message())

	if(state == Connected)
		state = Initialized;

	if (onCloseCallback)
		onCloseCallback(shared_from_this());
}

void UdpAsyncClient::handle_connect(const boost::system::error_code& error)
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

void UdpAsyncClient::do_close()
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

void UdpAsyncClient::do_rewrite()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (state != Clear)
		send_message();
}

void UdpAsyncClient::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	messageBuffer = data;

	if (state != Clear)
		send_message();
}

void UdpAsyncClient::send_message()
{
	if (state == Initialized)
	{
		if (implicitPort && !socket.is_open())
		{
			boost::system::error_code ec;

			socket.open(target_endpoint.protocol(), ec);
			socket.set_option(boost::asio::socket_base::reuse_address(true),ec);
			socket.bind(udp::endpoint(target_endpoint.protocol(), implicitPort), ec);

			if (ec)
			{
				UDP_LOG("port bind error: " << ec.message());
				return;
			}
		}

		socket.async_connect(target_endpoint, std::bind(&UdpAsyncClient::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if (state == Connected)
	{
		if (!messageBuffer.empty())
		{
			UDP_LOG("writing " << messageBuffer.size() << " bytes");

			socket.async_send_to(boost::asio::buffer(messageBuffer.data(), messageBuffer.size()), target_endpoint,
				std::bind(&UdpAsyncClient::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}
}

void UdpAsyncClient::handle_write(const boost::system::error_code& error, size_t sz)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (error)
	{
		postFail("Write", error);
	}
}


std::string UdpAsyncClient::getName()
{
	return target_endpoint.address().to_string() + ":" + std::to_string(target_endpoint.port());
}

udp::endpoint& UdpAsyncClient::getEndpoint()
{
	return target_endpoint;
}

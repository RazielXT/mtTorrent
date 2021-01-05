#include "UdpAsyncWriter.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG(LogTypeUdp, x) //{std::stringstream ss; ss << getName() << " " << x; WriteLogImplementation(LogTypeUdp, ss);}

UdpAsyncWriter::UdpAsyncWriter(asio::io_service& io) : io_service(io), socket(io)
{
}

UdpAsyncWriter::~UdpAsyncWriter()
{
}

void UdpAsyncWriter::setAddress(const Addr& addr)
{
	target_endpoint = addr.toUdpEndpoint();

	state = Initialized;
}

void UdpAsyncWriter::setAddress(const std::string& host, const std::string& p)
{
	hostname = host;
	port = p;

	udp::resolver::query query(hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncWriter::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncWriter::setAddress(const std::string& host, const std::string& p, bool ipv6)
{
	hostname = host;
	port = p;

	udp::resolver::query query(ipv6 ? udp::v6() : udp::v4(), hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncWriter::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncWriter::setAddress(const udp::endpoint& addr)
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
	onResponse = nullptr;
	onCloseCallback = nullptr;

	io_service.post(std::bind(&UdpAsyncWriter::do_close, shared_from_this()));
}

void UdpAsyncWriter::write(const DataBuffer& data)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	messageBuffer.store(data.data(), data.size());

	io_service.post(std::bind(&UdpAsyncWriter::do_write, shared_from_this()));
}

void UdpAsyncWriter::write()
{
	io_service.post(std::bind(&UdpAsyncWriter::do_rewrite, shared_from_this()));
}

extern void recordUtpPacket(const BufferView& data);

void UdpAsyncWriter::write(const BufferView& data, WriteOption option)
{
	//recordUtpPacket(data);

	//std::lock_guard<std::mutex> guard(stateMutex);

	if (state == Connected)
		send_message(data, option);
	else
	{
		messageBuffer = data;
		io_service.post(std::bind(&UdpAsyncWriter::do_write, shared_from_this()));
	}
}

void UdpAsyncWriter::handle_resolve(const std::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		target_endpoint = *iterator;
		state = Initialized;

		if (messageBuffer.size)
			send_message();
	}
	else
	{
		postFail("Resolve", error);
	}
}

void UdpAsyncWriter::postFail(std::string place, const std::error_code& error)
{
	if(error)
		UDP_LOG(place << "-" << error.message())

	if(state == Connected)
		state = Initialized;

	if (onCloseCallback)
		onCloseCallback(shared_from_this());
}

void UdpAsyncWriter::handle_connect(const std::error_code& error)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		state = Connected;

		if (onResponse)
			socket.async_receive(asio::null_buffers(), std::bind(&UdpAsyncWriter::handle_receive, shared_from_this(), std::placeholders::_1));

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

	onResponse = nullptr;
	onCloseCallback = nullptr;

	if (state == Connected)
		state = Initialized;

	std::error_code error;

	if (socket.is_open())
	{
		socket.shutdown(asio::socket_base::shutdown_both);
		socket.close(error);
	}	
}

void UdpAsyncWriter::do_rewrite()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (state != Clear)
		send_message();
}

void UdpAsyncWriter::do_write()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (state != Clear)
	{
		send_message();
	}
	else if (!hostname.empty())
	{
		setAddress(hostname, port);
	}
}

void UdpAsyncWriter::send_message()
{
	if (state == Initialized)
	{
		if (bindPort && !socket.is_open())
		{
			std::error_code ec;

			socket.open(target_endpoint.protocol(), ec);
			socket.set_option(asio::socket_base::reuse_address(true),ec);
			socket.bind(udp::endpoint(target_endpoint.protocol(), bindPort), ec);
			socket.non_blocking(true, ec);

			if (ec)
			{
				UDP_LOG("port bind error: " << ec.message());
			}
		}

		socket.async_connect(target_endpoint, std::bind(&UdpAsyncWriter::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if (state == Connected)
	{
		if (messageBuffer.size)
		{
			send_message(messageBuffer);
		}
	}
}

struct dont_fragment
{
#ifdef _WIN32
	explicit dont_fragment(bool val) : m_value(val) {}
	template<class Protocol>
	int name(Protocol const&) const { return IP_DONTFRAGMENT; }
	template<class Protocol>
	int const* data(Protocol const&) const { return &m_value; }
#else
	explicit dont_fragment(bool val) { m_value = val ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT; }
	template<class Protocol>
	int name(Protocol const&) const { return IP_MTU_DISCOVER; }
	template<class Protocol>
	int const* data(Protocol const&) const { return &m_value; }
#endif

	template<class Protocol>
	int level(Protocol const&) const { return IPPROTO_IP; }
	template<class Protocol>
	size_t size(Protocol const&) const { return sizeof(m_value); }
	int m_value;
};

void UdpAsyncWriter::send_message(const BufferView& buffer, WriteOption opt)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (opt == WriteOption::DontFragment)
	{
		std::error_code ec;
		socket.set_option(dont_fragment(true), ec);
	}

	send_message(buffer);

	if (opt == WriteOption::DontFragment)
	{
		std::error_code ec;
		socket.set_option(dont_fragment(false), ec);
	}
}

void UdpAsyncWriter::send_message(const BufferView& buffer)
{
	UDP_LOG("writing " << buffer.size << " bytes");

	std::error_code ec;
	socket.send_to(asio::buffer(buffer.data, buffer.size), target_endpoint, {}, ec);

	if (ec)
		postFail("Write", ec);
}

std::string UdpAsyncWriter::getName()
{
	return target_endpoint.address().to_string() + ":" + std::to_string(target_endpoint.port());
}

udp::endpoint& UdpAsyncWriter::getEndpoint()
{
	return target_endpoint;
}

void UdpAsyncWriter::handle_receive(const std::error_code& error)
{
	UDP_LOG("handle_receive status: " << error.message());

	if (error || state != Connected)
		return;

	readSocket();
	socket.async_receive(asio::null_buffers(), std::bind(&UdpAsyncWriter::handle_receive, shared_from_this(), std::placeholders::_1));
}

void UdpAsyncWriter::readSocket()
{
	for (size_t i = 0; i < 30; i++)
	{
		std::error_code ec;
		auto av = socket.available(ec);
		if (!av)
			break;

		receiveBuffer.resize(av);
		size_t transferred = socket.receive_from(asio::buffer(receiveBuffer), target_endpoint, 0, ec);

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
			onResponse(shared_from_this(), &receiveBuffer);
			UDP_LOG("readSocket received " << receiveBuffer.size());
		}
	}
}

#include "UdpAsyncWriter.h"

#define UDP_LOG(x) WRITE_LOG(x)

UdpAsyncWriter::UdpAsyncWriter(asio::io_context& io) : io_context(io), socket(io)
{
	CREATE_LOG(UdpWriter);
}

UdpAsyncWriter::~UdpAsyncWriter()
{
}

void UdpAsyncWriter::setAddress(const Addr& addr)
{
	target_endpoint = addr.toUdpEndpoint();
	hostname = target_endpoint.address().to_string();
	NAME_LOG(getName());

	state = Initialized;
}

void UdpAsyncWriter::setAddress(const udp::endpoint& addr)
{
	target_endpoint = addr;
	hostname = target_endpoint.address().to_string();
	NAME_LOG(getName());

	state = Initialized;
}

void UdpAsyncWriter::setAddress(const std::string& host, const std::string& p)
{
	hostname = host;
	port = p;

	NAME_LOG(getName())
	UDP_LOG(host << " host");

	resolveHostname();
}

void UdpAsyncWriter::setBindAddress(const asio::ip::address& addr)
{
	bindAddress = addr;
}

void UdpAsyncWriter::setBindPort(uint16_t port)
{
	bindPort = port;
}

void UdpAsyncWriter::setBroadcast(bool b)
{
	broadcast = b;
}

void UdpAsyncWriter::close()
{
	asio::post(io_context, std::bind(&UdpAsyncWriter::do_close, shared_from_this()));
}

void UdpAsyncWriter::write(const DataBuffer& data)
{
	asio::post(io_context, std::bind(&UdpAsyncWriter::do_write, shared_from_this(), data));
}

void UdpAsyncWriter::write()
{
	asio::post(io_context, std::bind(&UdpAsyncWriter::do_rewrite, shared_from_this()));
}

void UdpAsyncWriter::write(const BufferView& data, WriteOption option)
{
	if (state == Connected)
		send_message(data, option);
	else
	{
		asio::post(io_context, std::bind(&UdpAsyncWriter::do_write, shared_from_this(), DataBuffer(data.data, data.data + data.size)));
	}
}

void UdpAsyncWriter::resolveHostname()
{
	if (resolving || hostname.empty())
		return;

	UDP_LOG("resolveHostname");

	resolving = true;

	auto resolver = std::make_shared<udp::resolver>(io_context);
	resolver->async_resolve(hostname, port, std::bind(&UdpAsyncWriter::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncWriter::handle_resolve(const std::error_code& error, udp::resolver::results_type results, std::shared_ptr<udp::resolver> resolver)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	UDP_LOG("handle_resolve");

	if (!error && !results.empty())
	{
		target_endpoint = results.begin()->endpoint();
		state = Initialized;

		if (!messageBuffer.empty())
			send_message();
	}
	else
	{
		postFail("Resolve", error);
	}

	resolving = false;
}

void UdpAsyncWriter::postFail(std::string place, const std::error_code& error)
{
	if (error)
		UDP_LOG(place << "-" << error.message())

	if (state == Connected)
		state = Initialized;

	if (onCloseCallback)
		onCloseCallback(shared_from_this());
}

void UdpAsyncWriter::handle_connect(const std::error_code& error)
{
	UDP_LOG("handle_connect");

	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		setConnected();
	}
	else
	{
		postFail("Connect", error);
	}
}

void UdpAsyncWriter::setConnected()
{
	if (onResponse && state != Connected)
	{
		UDP_LOG("call async_receive");
		socket.async_receive(asio::null_buffers(), std::bind(&UdpAsyncWriter::handle_receive, shared_from_this(), std::placeholders::_1));
	}

	state = Connected;

	send_message();
}

void UdpAsyncWriter::do_close()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	onResponse = nullptr;
	onCloseCallback = nullptr;

	if (state == Connected)
		state = Initialized;

	if (socket.is_open())
	{
		std::error_code error;
		socket.shutdown(asio::socket_base::shutdown_both, error);
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

	messageBuffer = std::move(data);

	if (state != Clear)
	{
		send_message();
	}
	else if (!hostname.empty())
	{
		resolveHostname();
	}
}

void UdpAsyncWriter::send_message()
{
	if (state == Initialized)
	{
		if (!socket.is_open())
		{
			std::error_code ec;

			socket.open(target_endpoint.protocol(), ec);
			socket.set_option(asio::socket_base::reuse_address(true),ec);
			socket.set_option(asio::socket_base::receive_buffer_size(4000000), ec);
			//socket.set_option(asio::socket_base::send_buffer_size(1000000), ec);

			if (!bindAddress.is_unspecified())
				socket.bind(udp::endpoint(bindAddress, bindPort), ec);
			else if (bindPort)
				socket.bind(udp::endpoint(target_endpoint.protocol(), bindPort), ec);

			socket.non_blocking(true, ec);

			if (ec)
			{
				UDP_LOG("port bind error: " << ec.message());
			}
		}

		if (broadcast)
			setConnected();
		else
			socket.async_connect(target_endpoint, std::bind(&UdpAsyncWriter::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if (state == Connected)
	{
		if (!messageBuffer.empty())
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

	UDP_LOG("writing finished");

	if (ec)
		postFail("Write", ec);
}

std::string UdpAsyncWriter::getName() const
{
	return hostname;
}

const udp::endpoint& UdpAsyncWriter::getEndpoint() const
{
	return target_endpoint;
}

void UdpAsyncWriter::handle_receive(const std::error_code& error)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	UDP_LOG("handle_receive status: " << error.message());

	if (error || state != Connected)
		return;

	readSocket();

	UDP_LOG("call async_receive");
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
				UDP_LOG("receive error: " << ec.message());

			break;
		}

		if (transferred)
		{
			receiveBuffer.resize(transferred);
			onResponse(shared_from_this(), receiveBuffer);
			UDP_LOG("readSocket received " << transferred);
		}
	}
}

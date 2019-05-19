#include "TcpAsyncStream.h"
#include "Logging.h"

#define TCP_LOG(x) WRITE_LOG(LogTypeTcp, x)

TcpAsyncStream::TcpAsyncStream(asio::io_service& io) : io_service(io), socket(io), timeoutTimer(io)
{
}

TcpAsyncStream::~TcpAsyncStream()
{
}

void TcpAsyncStream::init(const std::string& hostname, const std::string& port)
{
	info.host = hostname;
	info.port = (uint16_t)strtoul(port.data(), NULL, 10);
}

void TcpAsyncStream::connect(const std::string& hostname, const std::string& port)
{
	if (state != Disconnected)
		return;

	init(hostname, port);
	connectByHostname();
}

void TcpAsyncStream::connect(const uint8_t* ip, uint16_t port, bool ipv6)
{
	if (state != Disconnected)
		return;

	info.endpoint = ipv6 ? tcp::endpoint(asio::ip::address_v6(*reinterpret_cast<const asio::ip::address_v6::bytes_type*>(ip)), port) :
		tcp::endpoint(asio::ip::address_v4(*reinterpret_cast<const asio::ip::address_v4::bytes_type*>(ip)), port);
	info.endpointInitialized = true;

	info.host = info.endpoint.address().to_string();
	info.port = port;

	timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));
	timeoutTimer.expires_from_now(std::chrono::seconds(5));

	connectEndpoint();
}

void TcpAsyncStream::connect(const std::string& ip, uint16_t port)
{
	if (state != Disconnected)
		return;

	info.endpoint = tcp::endpoint(asio::ip::address::from_string(ip), port);
	info.endpointInitialized = true;

	info.host = ip;
	info.port = port;

	connectEndpoint();
}

void TcpAsyncStream::close()
{
	onConnectCallback = nullptr;
	onReceiveCallback = nullptr;
	onCloseCallback = nullptr;

	if (state == Disconnected)
		return;

	io_service.post(std::bind(&TcpAsyncStream::do_close, shared_from_this()));
}

void TcpAsyncStream::write(const DataBuffer& data)
{
	io_service.post(std::bind(&TcpAsyncStream::do_write, shared_from_this(), data));
}

DataBuffer TcpAsyncStream::getReceivedData()
{
	std::lock_guard<std::mutex> guard(receiveBuffer_mutex);

	return receiveBuffer;
}

void TcpAsyncStream::consumeData(size_t size)
{
	std::lock_guard<std::mutex> guard(receiveBuffer_mutex);

	receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + size);
}

uint16_t TcpAsyncStream::getPort()
{
	return info.port;
}

std::string& TcpAsyncStream::getHostname()
{
	return info.host;
}

tcp::endpoint& TcpAsyncStream::getEndpoint()
{
	return info.endpoint;
}

void TcpAsyncStream::connectByHostname()
{
	state = Connecting;

	tcp::resolver::query query(info.host, std::to_string(info.port));

	auto resolver = std::make_shared<tcp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&TcpAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void TcpAsyncStream::connectEndpoint()
{
	state = Connecting;

	socket.async_connect(info.endpoint, std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncStream::setAsConnected()
{
	state = Connected;
	info.endpoint = socket.remote_endpoint();
	info.endpointInitialized = true;

	socket.async_receive(asio::buffer(recv_buffer), std::bind(&TcpAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onConnectCallback)
			onConnectCallback();
	}
}

void TcpAsyncStream::postFail(std::string place, const std::error_code& error)
{
	if (state == Disconnected)
		return;

	try
	{
		if (socket.is_open())
			socket.close();
	}
	catch (...)
	{
	}

	if(error)
		TCP_LOG("error on " << place << ": " << error.message())
	else
		TCP_LOG("end on " << place);

	state = Disconnected;
	timeoutTimer.cancel();

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onCloseCallback)
			onCloseCallback(error.value());

		onConnectCallback = nullptr;
		onCloseCallback = nullptr;
		onReceiveCallback = nullptr;
	}
}

void TcpAsyncStream::handle_resolve(const std::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
{
	if (!error)
	{
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, ++iterator, resolver));
	}
	else
	{
		postFail("Resolve", error);
	}
}

void TcpAsyncStream::handle_resolver_connect(const std::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
{
	if (error && iterator != tcp::resolver::iterator())
	{
		socket.close();
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, ++iterator, resolver));
	}
	else
	{
		if (!error)
		{
			info.endpoint = socket.remote_endpoint();
			info.endpointInitialized = true;
		}

		handle_connect(error);
	}
}

void TcpAsyncStream::handle_connect(const std::error_code& error)
{
	if (!error)
	{
		setAsConnected();
	}
	else
	{
		postFail("Connect", error);
	}
}

void TcpAsyncStream::do_close()
{
	postFail("Close", std::error_code());
}

void TcpAsyncStream::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	write_msgs.push_back(data);

	if (state == Connected)
	{
		TCP_LOG("writing " << data.size() << " bytes");

		bool write_in_progress = write_msgs.size() > 1;

		if (!write_in_progress)
		{
			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else if (state != Connecting)
	{
		if (info.endpointInitialized)
		{
			connectEndpoint();
		}
		else if (!info.host.empty())
		{
			connectByHostname();
		}
	}
}

void TcpAsyncStream::handle_write(const std::error_code& error)
{
	if (!error)
	{
		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		write_msgs.pop_front();

		if (!write_msgs.empty())
		{
			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

void TcpAsyncStream::handle_receive(const std::error_code& error, std::size_t bytes_transferred)
{
	TCP_LOG("received " << bytes_transferred << " bytes");

	if (!error)
	{
		appendData(recv_buffer.data(), bytes_transferred);

		timeoutTimer.expires_from_now(std::chrono::seconds(60));
		timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));

		socket.async_receive(asio::buffer(recv_buffer),
			std::bind(&TcpAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

		{
			std::lock_guard<std::mutex> guard(callbackMutex);

			if (onReceiveCallback)
				onReceiveCallback();
		}
	}
	else
	{
		postFail("Receive", error);
	}
}

void TcpAsyncStream::appendData(char* data, size_t size)
{
	std::lock_guard<std::mutex> guard(receiveBuffer_mutex);

	receiveBuffer.insert(receiveBuffer.end(), data, data + size);
}

void TcpAsyncStream::checkTimeout(const asio::error_code& error)
{
	if (state == Disconnected || error)
		return;

	postFail("timeout", std::error_code());
}

#include "TcpAsyncStream.h"
#include "Logging.h"

#define TCP_LOG(x) WRITE_LOG("TCP: " << info.host << " " << x)

TcpAsyncStream::TcpAsyncStream(boost::asio::io_service& io) : io_service(io), socket(io), timeoutTimer(io)
{
}

TcpAsyncStream::~TcpAsyncStream()
{
}

void TcpAsyncStream::init(const std::string& hostname, const std::string& port)
{
	if (state != Disconnected)
		return;

	info.host = hostname;
	info.port = port;
	info.hostInitialized = true;
}

void TcpAsyncStream::connect(const std::string& hostname, const std::string& port)
{
	if (state != Disconnected)
		return;

	info.host = hostname;
	state = Connecting;
	
	tcp::resolver::query query(hostname, port);

	auto resolver = std::make_shared<tcp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&TcpAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void TcpAsyncStream::connect(const uint8_t* ip, uint16_t port, bool ipv6)
{
	if (state != Disconnected)
		return;

	auto endpoint = ipv6 ? tcp::endpoint(boost::asio::ip::address_v6(*reinterpret_cast<const boost::asio::ip::address_v6::bytes_type*>(ip)), port) :
		tcp::endpoint(boost::asio::ip::address_v4(*reinterpret_cast<const boost::asio::ip::address_v4::bytes_type*>(ip)), port);
	info.host = endpoint.address().to_string();
	state = Connecting;

	TCP_LOG("connecting");

	timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this()));
	timeoutTimer.expires_from_now(boost::posix_time::seconds(5));

	socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncStream::connect(const std::string& ip, uint16_t port)
{
	if (state != Disconnected)
		return;

	info.host = ip;
	state = Connecting;

	tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
	socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncStream::close()
{
	//std::lock_guard<std::mutex> guard(callbackMutex);

	onConnectCallback = nullptr;
	onReceiveCallback = nullptr;
	onCloseCallback = nullptr;

	if (state == Disconnected)
		return;

	io_service.post(std::bind(&TcpAsyncStream::do_close, shared_from_this()));
}

void TcpAsyncStream::write(const DataBuffer& data)
{
	io_service.post(std::bind(&TcpAsyncStream::do_write, this, data));
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

std::string TcpAsyncStream::getName()
{
	return info.host;
}

tcp::endpoint& TcpAsyncStream::getEndpoint()
{
	return info.endpoint;
}

void TcpAsyncStream::setAsConnected()
{
	state = Connected;

	socket.async_receive(boost::asio::buffer(recv_buffer), std::bind(&TcpAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

	check_write();

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onConnectCallback)
			onConnectCallback();
	}
}

void TcpAsyncStream::postFail(std::string place, const boost::system::error_code& error)
{
	if (state == Disconnected)
		return;

	if (socket.is_open())
		socket.close();

	if(error)
		TCP_LOG("error on " << place << ": " << error.message())
	else
		TCP_LOG("end on " << place);

	state = Disconnected;
	timeoutTimer.cancel();

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onCloseCallback)
			onCloseCallback();
	}
}

void TcpAsyncStream::handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
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

void TcpAsyncStream::handle_resolver_connect(const boost::system::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
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
			info.endInitialized = true;
			info.endpoint = socket.remote_endpoint();
		}

		handle_connect(error);
	}
}

void TcpAsyncStream::handle_connect(const boost::system::error_code& error)
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
	postFail("Close", boost::system::error_code());
}

void TcpAsyncStream::check_write()
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	if (!write_msgs.empty())
	{
		boost::asio::async_write(socket,
			boost::asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
			std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
	}
}

void TcpAsyncStream::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	if (state != Disconnected)
	{
		TCP_LOG("writing " << data.size() << " bytes");

		bool write_in_progress = !write_msgs.empty();
		write_msgs.push_back(data);
		if (!write_in_progress)
		{
			boost::asio::async_write(socket,
				boost::asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else if (info.endInitialized)
	{
		write_msgs.push_back(data);
		state = Connecting;

		socket.async_connect(info.endpoint, std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if(info.hostInitialized)
	{
		write_msgs.push_back(data);
		state = Connecting;

		tcp::resolver::query query(info.host, info.port);
		auto resolver = std::make_shared<tcp::resolver>(io_service);
		resolver->async_resolve(query, std::bind(&TcpAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
	}

}

void TcpAsyncStream::handle_write(const boost::system::error_code& error)
{
	if (!error)
	{
		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		write_msgs.pop_front();
		if (!write_msgs.empty())
		{
			boost::asio::async_write(socket,
				boost::asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

void TcpAsyncStream::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	TCP_LOG("received " << bytes_transferred << " bytes");

	if (!error)
	{
		appendData(recv_buffer.data(), bytes_transferred);

		timeoutTimer.expires_from_now(boost::posix_time::seconds(60));
		socket.async_receive(boost::asio::buffer(recv_buffer),
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

void TcpAsyncStream::checkTimeout()
{
	if (state == Disconnected)
		return;

	if (timeoutTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		timeoutTimer.expires_at(boost::posix_time::pos_infin);
		postFail("timeout", boost::system::error_code());
	}

	timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this()));
}

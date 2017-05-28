#include "TcpAsyncStream.h"
#include <iostream>

TcpAsyncStream::TcpAsyncStream(boost::asio::io_service& io) : io_service(io), resolver(io), socket(io)
{
	
}

void TcpAsyncStream::connect(const std::string& hostname, const std::string& port)
{
	if (state != Disconnected)
		return;

	host = hostname;
	state = Connecting;
	
	tcp::resolver::query query(hostname, port);
	resolver.async_resolve(query,
		std::bind(&TcpAsyncStream::handle_resolve, this,
			std::placeholders::_1,
			std::placeholders::_2));
}

void TcpAsyncStream::close()
{
	io_service.post(std::bind(&TcpAsyncStream::do_close, this));
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

void TcpAsyncStream::setOnConnectCallback(std::function<void()> func)
{
	onConnectCallback = func;
}

void TcpAsyncStream::setOnReceiveCallback(std::function<void()> func)
{
	onReceiveCallback = func;
}

void TcpAsyncStream::setOnCloseCallback(std::function<void()> func)
{
	onCloseCallback = func;
}

void TcpAsyncStream::postFail(std::string place, const boost::system::error_code& error)
{
	//std::cout << place << "-" << host << "-" << error.message() << "\n";

	if (onCloseCallback)
		onCloseCallback();
}

void TcpAsyncStream::handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator iterator)
{
	if (!error)
	{
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint,
			std::bind(&TcpAsyncStream::handle_connect, this,
				std::placeholders::_1, ++iterator));
	}
	else
	{
		state = Disconnected;
		postFail("Resolve", error);
	}
}

void TcpAsyncStream::handle_connect(const boost::system::error_code& error, tcp::resolver::iterator iterator)
{
	if (!error)
	{
		state = Connected;

		socket.async_receive(boost::asio::buffer(recv_buffer),
			std::bind(&TcpAsyncStream::handle_receive, this,
				std::placeholders::_1,
				std::placeholders::_2));

		if (onConnectCallback)
			onConnectCallback();
	}
	else if (iterator != tcp::resolver::iterator())
	{
		socket.close();
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint,
			std::bind(&TcpAsyncStream::handle_connect, this,
				std::placeholders::_1, ++iterator));
	}
	else
	{
		state = Disconnected;
		postFail("Connect", error);
	}
}

void TcpAsyncStream::do_close()
{
	boost::system::error_code error;

	if (socket.is_open())
		socket.close(error);

	state = Disconnected;

	if(error)
		postFail("Close", error);
}

void TcpAsyncStream::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	bool write_in_progress = !write_msgs.empty();
	write_msgs.push_back(data);
	if (!write_in_progress)
	{
		boost::asio::async_write(socket,
			boost::asio::buffer(write_msgs.front().data(),
				write_msgs.front().size()),
			std::bind(&TcpAsyncStream::handle_write, this,
				std::placeholders::_1));
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
				boost::asio::buffer(write_msgs.front().data(),
					write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, this,
					std::placeholders::_1));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

void TcpAsyncStream::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
	socket.async_receive(boost::asio::buffer(recv_buffer),
		std::bind(&TcpAsyncStream::handle_receive, this,
			std::placeholders::_1,
			std::placeholders::_2));

	appendData(recv_buffer.data(), bytes_transferred);

	if (onReceiveCallback)
		onReceiveCallback();
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

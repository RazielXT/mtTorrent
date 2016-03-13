#include "TcpStream.h"
#include <iostream>
#include "Interface.h"
#include <boost/asio/use_future.hpp>

TcpStream::TcpStream() : resolver(io_service)
{

}

void TcpStream::setTarget(const char* hostname, const char* p)
{ 
	host = hostname;
	port = p;
}

void TcpStream::setTimeout(int32_t msTimeout)
{
	timeout = msTimeout;
	configureSocket();
}

bool TcpStream::active()
{
	return connected();
}

void TcpStream::close()
{
	std::lock_guard<std::mutex> guard(socket_mutex);

	socket->close();
}

void TcpStream::ensureConnection()
{
	std::lock_guard<std::mutex> guard(socket_mutex);

	if (!socket || !connected())
	{
		socket = std::make_unique<tcp::socket>(io_service);
		connect(host.c_str(), port.c_str());
		configureSocket();
	}
}

void TcpStream::configureSocket()
{
	if (socket && timeout>0)
	{
		//auto r = setsockopt(socket->native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		//r =setsockopt(socket->native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	}
}

#define ASYNCTEST
//#define ASYNCTEST_H

void TcpStream::connect(const char* host, const char* port)
{
	openTcpSocket(*socket, resolver, host, port);

#ifdef  ASYNCTEST_H

	socket->async_receive(
		boost::asio::buffer(recv_buf),	std::bind(&TcpStream::handleReceive, this,
			std::placeholders::_1,
			std::placeholders::_2));

	std::thread([this]() { io_service.run(); }).detach();

#else
	std::thread(&TcpStream::socketListening, this).detach();
#endif
}

bool TcpStream::connected()
{
	return socket->is_open();
}

void TcpStream::write(DataBuffer data)
{
	boost::asio::streambuf dBuffer;
	dBuffer.sputn(data.data(), data.size());

	try
	{
		ensureConnection();

		boost::asio::write(*socket, dBuffer);
	}
	catch (const std::exception&e)
	{
		std::cout << host << " Socket write exception: " << e.what() << "\n";
		close();
	}
}

#ifndef  ASYNCTEST

void TcpStream::socketListening()
{
	while (connected())
	{
		blockingRead();
	}
}

void TcpStream::blockingRead()
{
	boost::asio::streambuf response;
	boost::system::error_code error;

	size_t len = boost::asio::read(*socket, response, error);

	if (len > 0)
	{
		std::istream response_stream(&response);
		DataBuffer buffer;

		buffer.resize(len);
		response_stream.read(&buffer[0], len);

		appendData(buffer);
	}
	else
	{
		std::cout << "Socket listening exception: " << error.message() << "\n";
		close();
	}
}
#else

void TcpStream::socketListening()
{
	while (connected())
	{		
		blockingRead();
	}
}

void TcpStream::blockingRead()
{
	std::array<char, 1024> recv_buf;
	std::future<std::size_t> read_result = socket->async_receive(boost::asio::buffer(recv_buf), boost::asio::use_future);

	std::thread([this]() { io_service.run(); }).detach();

	if (read_result.wait_for(std::chrono::seconds(timeout)) == std::future_status::timeout)
	{
		std::cout << host << " Socket timeout\n";
		close();
	}
	else
	{
		try
		{
			auto bytes_transferred = read_result.get();

			std::cout << host << " Bytes: " << std::to_string(bytes_transferred) << "\n";

			if (bytes_transferred > 0)
			{
				DataBuffer buffer(recv_buf.begin(), recv_buf.begin() + bytes_transferred);
				appendData(buffer);

				std::cout << host << " Read " << getTimestamp() << "\n";
			}
		}
		catch (const std::exception&e)
		{
			std::cout << host << " socket listening exception: " << e.what() << "\n";
			close();
		}
	}
}

#endif // ! ASYNCTEST

DataBuffer TcpStream::getReceivedData()
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	return buffer;
}

void TcpStream::appendData(DataBuffer& data)
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	buffer.insert(buffer.end(), data.begin(), data.end());
}

void TcpStream::consumeData(size_t size)
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	buffer.erase(buffer.begin(), buffer.begin() + size);
}

void TcpStream::resetData()
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	buffer.clear();
}

DataBuffer TcpStream::sendBlockingRequest(DataBuffer& data)
{
	try
	{
		if (!active())
			openTcpSocket(*socket, resolver, host.c_str(), port.c_str());

		return sendTcpRequest(*socket, data);
	}
	catch (const std::exception&e)
	{
		std::cout << "Socket request exception: " << e.what() << "\n";
		close();
	}

	return{};
}

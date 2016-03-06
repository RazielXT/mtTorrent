#include "TcpStream.h"

void TcpStream::connect(const char* server, const char* port)
{
	openTcpSocket(socket, resolver, server, port);
}

void TcpStream::write(std::vector<char> data)
{
	boost::asio::streambuf dBuffer;
	dBuffer.sputn(data.data(), data.size());

	boost::asio::write(socket, dBuffer);
}

std::vector<char> TcpStream::getReceivedData()
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	return buffer;
}

void TcpStream::blockingRead()
{
	boost::asio::streambuf response;
	boost::system::error_code error;

	size_t len = boost::asio::read(socket, response, error);

	if (len > 0)
	{
		std::istream response_stream(&response);
		std::vector<char> buffer;

		buffer.resize(len);
		response_stream.read(&buffer[0], len);

		appendData(buffer);
	}
	else if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);
}

void TcpStream::appendData(std::vector<char>& data)
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

//-----------------------------------

TcpStreamAsync::~TcpStreamAsync()
{
	stopped = true;

	if (socket.is_open())
		socket.close();

	if (readHandle.valid())
		readHandle.get();
}

void TcpStreamAsync::connect(const char* server, const char* port)
{
	openTcpSocket(socket, resolver, server, port);

	readHandle = std::async(&TcpStreamAsync::startListening, this);
}

void TcpStreamAsync::stopListening()
{
	stopped = true;
}

void TcpStreamAsync::startListening()
{
	while (!stopped)
	{
		blockingRead();
	}
}

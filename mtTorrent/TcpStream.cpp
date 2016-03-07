#include "TcpStream.h"

TcpStream::TcpStream() : resolver(io_service)
{ 
	socket = std::make_unique<tcp::socket>(io_service);
}

void TcpStream::init(const char* server, const char* p)
{
	host = server;
	port = p;

	connect(server, p);
}

void TcpStream::connect(const char* server, const char* port)
{
	openTcpSocket(*socket, resolver, server, port);
}

void TcpStream::write(std::vector<char> data)
{
	boost::asio::streambuf dBuffer;
	dBuffer.sputn(data.data(), data.size());

	ensureConnection();
	boost::asio::write(*socket, dBuffer);
}

bool TcpStream::blockingRead()
{
	ensureConnection();

	boost::asio::streambuf response;
	boost::system::error_code error;

	size_t len = boost::asio::read(*socket, response, error);

	if (len > 0)
	{
		std::istream response_stream(&response);
		std::vector<char> buffer;

		buffer.resize(len);
		response_stream.read(&buffer[0], len);

		appendData(buffer);
	}
	else if (error == boost::asio::error::eof)
	{
		closeConnection();
		return false;
	}	
	else
		throw boost::system::system_error(error);

	return true;
}

std::vector<char> TcpStream::getReceivedData()
{
	std::lock_guard<std::mutex> guard(buffer_mutex);

	return buffer;
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

bool TcpStream::connected()
{
	return socket->is_open();
}

void TcpStream::closeConnection()
{
	std::lock_guard<std::mutex> guard(socket_mutex);

	socket->close();
}

void TcpStream::ensureConnection()
{
	std::lock_guard<std::mutex> guard(socket_mutex);

	if (!connected())
	{
		socket = std::make_unique<tcp::socket>(io_service);
		connect(host.c_str(), port.c_str());
	}	
}

//-----------------------------------

TcpStreamAsync::~TcpStreamAsync()
{
	if (socket->is_open())
		socket->close();

	if (readHandle.valid())
		readHandle.get();
}

void TcpStreamAsync::connect(const char* server, const char* port)
{
	openTcpSocket(*socket, resolver, server, port);

	readHandle = std::async(&TcpStreamAsync::startListening, this);
}

void TcpStreamAsync::startListening()
{
	while (connected())
	{
		blockingRead();
	}
}

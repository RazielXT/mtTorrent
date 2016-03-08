#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>

class TcpStream
{
public:

	TcpStream();

	void setTarget(const char* server, const char* port);

	void write(std::vector<char> data);

	std::vector<char> getReceivedData();
	void consumeData(size_t size);
	void resetData();

	bool active();
	void close();

protected:

	std::vector<char> buffer;
	std::mutex buffer_mutex;

	void socketListening();
	void blockingRead();
	void appendData(std::vector<char>& data);

	void connect(const char* server, const char* port);
	bool connected();
	
	void ensureConnection();

	std::mutex socket_mutex;
	boost::asio::io_service io_service;
	std::unique_ptr<tcp::socket> socket;
	tcp::resolver resolver;

	std::string host;
	std::string port;
};

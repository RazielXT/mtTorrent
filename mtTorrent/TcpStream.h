#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>

class TcpStream
{
public:

	TcpStream();

	void setTarget(const char* hostname, const char* port);
	void setTimeout(int32_t msTimeout);

	void write(std::vector<char> data);

	std::vector<char> getReceivedData();
	void consumeData(size_t size);
	void resetData();

	bool active();
	void close();

	//do not mix with async usage!!
	std::vector<char> sendBlockingRequest(std::vector<char>& data);

protected:

	std::vector<char> buffer;
	std::mutex buffer_mutex;

	void socketListening();
	void blockingRead();
	void appendData(std::vector<char>& data);

	void connect(const char* hostname, const char* port);
	bool connected();
	
	void ensureConnection();
	void configureSocket();

	std::mutex socket_mutex;
	boost::asio::io_service io_service;
	std::unique_ptr<tcp::socket> socket;
	tcp::resolver resolver;

	std::string host;
	std::string port;
	int32_t timeout = 5000;
};

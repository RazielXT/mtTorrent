#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>

class TcpStream
{
public:

	TcpStream();

	void setTarget(const char* hostname, const char* port);
	void setTimeout(int32_t msTimeout);

	void write(DataBuffer data);

	DataBuffer getReceivedData();
	void consumeData(size_t size);
	void resetData();

	bool active();
	void close();

	//do not mix with async usage!!
	DataBuffer sendBlockingRequest(DataBuffer& data);

protected:

	DataBuffer buffer;
	std::mutex buffer_mutex;

	//char recv_buf[128];
	//void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);

	void socketListening();
	void blockingRead();
	void appendData(DataBuffer& data);

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
	int32_t timeout = 15;
};

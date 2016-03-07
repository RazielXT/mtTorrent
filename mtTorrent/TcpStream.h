#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>

class TcpStream
{
public:

	TcpStream();

	void init(const char* server, const char* port);

	virtual void connect(const char* server, const char* port);

	bool blockingRead();
	void write(std::vector<char> data);

	std::vector<char> getReceivedData();
	void consumeData(size_t size);
	void resetData();

protected:

	bool connected();
	void closeConnection();
	void ensureConnection();
	std::mutex socket_mutex;

	void appendData(std::vector<char>& data);

	std::vector<char> buffer;
	std::mutex buffer_mutex;

	boost::asio::io_service io_service;
	std::unique_ptr<tcp::socket> socket;
	tcp::resolver resolver;

	std::string host;
	std::string port;
};

class TcpStreamAsync : public TcpStream
{
public:

	~TcpStreamAsync();

	virtual void connect(const char* server, const char* port);

protected:

	virtual void onReceiveAsync(std::vector<char> data) = 0;
	void startListening();

	std::future<void> readHandle;
};
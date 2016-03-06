#pragma once

#include "Network.h"
#include <mutex>
#include <future>

class TcpStream
{
public:

	TcpStream() : socket(io_service), resolver(io_service) {}

	virtual void connect(const char* server, const char* port);

	void blockingRead();
	void write(std::vector<char> data);

	std::vector<char> getReceivedData();
	void consumeData(size_t size);
	void resetData();

protected:

	void appendData(std::vector<char>& data);

	std::vector<char> buffer;
	std::mutex buffer_mutex;

	boost::asio::io_service io_service;
	tcp::socket socket;
	tcp::resolver resolver;

};

class TcpStreamAsync : public TcpStream
{
public:

	~TcpStreamAsync();

	virtual void connect(const char* server, const char* port);

protected:

	virtual void onReceiveAsync(std::vector<char> data) = 0;
	void stopListening();
	void startListening();

	std::future<void> readHandle;
	bool stopped = false;
};
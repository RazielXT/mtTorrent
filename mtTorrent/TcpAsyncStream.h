#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

class TcpAsyncStream
{
public:

	TcpAsyncStream(boost::asio::io_service& io_service);

	void connect(const std::string& hostname, const std::string& port);
	void close();

	void write(const DataBuffer& data);

	DataBuffer getReceivedData();
	void consumeData(size_t size);

	void setOnConnectCallback(std::function<void()> func);
	void setOnReceiveCallback(std::function<void()> func);
	void setOnCloseCallback(std::function<void()> func);

protected:

	void postFail(std::string place, const boost::system::error_code& error);

	enum { Disconnected, Connecting, Connected } state;
	void handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator iterator);
	void handle_connect(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);
	void do_close();

	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const boost::system::error_code& error);

	std::array<char, 1024> recv_buffer;
	void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
	void appendData(char* data, size_t size);
	std::mutex receiveBuffer_mutex;
	DataBuffer receiveBuffer;

	std::function<void()> onConnectCallback;
	std::function<void()> onReceiveCallback;
	std::function<void()> onCloseCallback;

	std::mutex socket_mutex;	
	tcp::socket socket;

	boost::asio::io_service& io_service;
	tcp::resolver resolver;

	std::string host;
	int32_t timeout = 15;
};

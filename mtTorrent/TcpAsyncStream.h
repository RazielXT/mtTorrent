#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

class TcpAsyncServer;

class TcpAsyncStream
{
	friend class TcpAsyncServer;

public:

	TcpAsyncStream(boost::asio::io_service& io_service);
	~TcpAsyncStream();

	void connect(const uint8_t* data, uint16_t port, bool ipv6);
	void connect(const std::string& ip, uint16_t port);
	void connect(const std::string& hostname, const std::string& port);

	void close();

	void write(const DataBuffer& data);

	DataBuffer getReceivedData();
	void consumeData(size_t size);

	std::function<void()> onConnectCallback;
	std::function<void()> onReceiveCallback;
	std::function<void()> onCloseCallback;

protected:

	void setAsConnected();

	void postFail(std::string place, const boost::system::error_code& error);

	enum { Disconnected, Connecting, Connected } state;
	void handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver);
	void handle_resolver_connect(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator, std::shared_ptr<tcp::resolver> resolver);
	void handle_connect(const boost::system::error_code& err);
	void do_close();

	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const boost::system::error_code& error);

	std::array<char, 10*1024> recv_buffer;
	void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
	void appendData(char* data, size_t size);
	std::mutex receiveBuffer_mutex;
	DataBuffer receiveBuffer;

	std::mutex socket_mutex;
	tcp::socket socket;

	void checkTimeout();
	boost::asio::deadline_timer timeoutTimer;

	boost::asio::io_service& io_service;

	std::string host;
	int32_t timeout = 15;
};

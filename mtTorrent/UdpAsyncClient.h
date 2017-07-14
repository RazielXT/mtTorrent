#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

class UdpAsyncClient
{
public:

	UdpAsyncClient(boost::asio::io_service& io_service, Addr& addr);

	void write(const DataBuffer& data);
	void close();

	std::function<void(DataBuffer&)> onReceiveCallback;
	std::function<void()> onCloseCallback;

protected:

	enum { Disconnected, Connected } state = Disconnected;

	void setAsConnected();

	void postFail(std::string place, const boost::system::error_code& error);

	void do_close();
	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const boost::system::error_code& error, size_t sz);

	std::array<char, 5 * 1024> recv_buffer;
	void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

	std::mutex socket_mutex;
	udp::socket socket;

	boost::asio::io_service& io_service;
	udp::endpoint target_endpoint;

};

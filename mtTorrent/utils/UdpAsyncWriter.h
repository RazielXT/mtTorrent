#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

class UdpAsyncReceiver;
class UdpAsyncWriter;
using UdpRequest = std::shared_ptr<UdpAsyncWriter>;

class UdpAsyncWriter : public std::enable_shared_from_this<UdpAsyncWriter>
{
	friend class UdpAsyncReceiver;

public:

	UdpAsyncWriter(asio::io_service& io_service);
	~UdpAsyncWriter();

	void setAddress(const Addr& addr);
	void setAddress(const udp::endpoint& addr);
	void setAddress(const std::string& hostname, const std::string& port);
	void setAddress(const std::string& hostname, const std::string& port, bool ipv6);
	void setBindPort(uint16_t port);

	std::string getName();
	udp::endpoint& getEndpoint();

	enum class WriteOption { None, DontFragment };
	void write(const BufferView& data, WriteOption opt = WriteOption::None );
	void write(const DataBuffer& data);
	void write();
	void close();

	std::function<void(UdpRequest, DataBuffer*)> onResponse;
	std::function<void(UdpRequest)> onCloseCallback;

protected:

	std::mutex stateMutex;
	enum { Clear, Initialized, Connected } state = Clear;

	void postFail(std::string place, const std::error_code& error);

	void handle_resolve(const std::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver);
	void handle_connect(const std::error_code& err);
	
	void do_write();
	void do_rewrite();
	void do_close();

	BufferView messageBuffer;

	void send_message();
	void send_message(const BufferView&);
	void send_message(const BufferView&, WriteOption opt);

	void handle_write(const std::error_code& error, size_t sz, WriteOption opt);

	void handle_receive(const std::error_code& error);
	void readSocket();
	DataBuffer receiveBuffer;

	uint16_t bindPort = 0;

	udp::endpoint target_endpoint;
	udp::socket socket;
	asio::io_service& io_service;

	std::string hostname;
	std::string port;

};

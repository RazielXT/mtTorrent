#pragma once

#include "Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

struct PackedUdpRequest;
using UdpRequest = std::shared_ptr<PackedUdpRequest>;

UdpRequest SendAsyncUdp(const std::string& hostname, const std::string& port, bool ipv6, DataBuffer& data, boost::asio::io_service& io, std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult);
UdpRequest SendAsyncUdp(Addr& addr, DataBuffer& data, boost::asio::io_service& io, std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult);

class UdpAsyncClient
{
public:

	UdpAsyncClient(boost::asio::io_service& io_service);
	~UdpAsyncClient();

	void setAddress(Addr& addr);
	void setAddress(const std::string& hostname, const std::string& port);
	void setAddress(const std::string& hostname, const std::string& port, bool ipv6);

	bool write(const DataBuffer& data);
	void close();

	std::function<void(DataBuffer&)> onReceiveCallback;
	std::function<void()> onCloseCallback;

protected:

	enum { Clear, Initialized, Connected } state = Clear;

	void handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver);
	void listenToResponse();
	bool listening = false;

	void postFail(std::string place, const boost::system::error_code& error);

	void handle_connect(const boost::system::error_code& err);
	void do_close();
	void do_write();
	
	DataBuffer messageBuffer;
	DataBuffer responseBuffer;
	void handle_write(const boost::system::error_code& error, size_t sz);
	void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

	void checkTimeout();
	boost::asio::deadline_timer timeoutTimer;
	uint8_t writeRetries = 0;

	udp::endpoint target_endpoint;
	udp::socket socket;
	boost::asio::io_service& io_service;

};

struct PackedUdpRequest
{
	PackedUdpRequest(boost::asio::io_service& io);

	bool write(DataBuffer& data);
	UdpAsyncClient client;
	void onFail();
	void onSuccess(DataBuffer&);
	std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult;
};

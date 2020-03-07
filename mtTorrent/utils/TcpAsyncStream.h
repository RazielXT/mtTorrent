#pragma once

#include "utils\Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>

class TcpAsyncServer;

class TcpAsyncStream : public std::enable_shared_from_this<TcpAsyncStream>
{
	friend class TcpAsyncServer;

public:

	TcpAsyncStream(asio::io_service& io_service);
	~TcpAsyncStream();

	void init(const std::string& hostname, const std::string& port);

	void connect(const uint8_t* data, uint16_t port, bool ipv6);
	void connect(const std::string& ip, uint16_t port);
	void connect(const std::string& hostname, const std::string& port);

	void close(bool immediate = true);

	void write(const DataBuffer& data);

	DataBuffer getReceivedData();
	void consumeData(size_t size);

	std::mutex callbackMutex;
	std::function<void()> onConnectCallback;
	std::function<void()> onReceiveCallback;
	std::function<void(int)> onCloseCallback;

	uint16_t getPort();
	std::string& getHostname();
	tcp::endpoint& getEndpoint();

	size_t getReceivedDataCount();

protected:

	void connectByHostname();
	void connectEndpoint();
	void setAsConnected();

	void postFail(std::string place, const std::error_code& error);

	enum { Disconnected, Connecting, Connected } state = Disconnected;
	void handle_resolve(const std::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver);
	void handle_resolver_connect(const std::error_code& err, tcp::resolver::iterator endpoint_iterator, std::shared_ptr<tcp::resolver> resolver);
	void handle_connect(const std::error_code& err);
	void do_close();

	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const std::error_code& error);

	std::array<char, 10*1024> recv_buffer;
	void handle_receive(const std::error_code& error, std::size_t bytes_transferred);
	void appendData(char* data, size_t size);
	std::mutex receiveBuffer_mutex;
	DataBuffer receiveBuffer;
	size_t receivedCounter = 0;

	std::mutex socket_mutex;
	tcp::socket socket;

	void checkTimeout(const asio::error_code& error);
	asio::steady_timer  timeoutTimer;

	asio::io_service& io_service;

	struct  
	{
		std::string host;
		uint16_t port;
		tcp::endpoint endpoint;
		bool endpointInitialized = false;
	}
	info;

	int32_t timeout = 15;
	bool writing = false;
};

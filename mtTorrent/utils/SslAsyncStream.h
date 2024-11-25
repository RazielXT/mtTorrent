#pragma once

#include "Network.h"
#include "Logging.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>

#ifdef MTT_WITH_SSL

class SslAsyncStream : public std::enable_shared_from_this<SslAsyncStream>
{
	friend class TcpAsyncServer;

public:

	SslAsyncStream(asio::io_context& io);
	~SslAsyncStream();

	void init(const std::string& hostname, const std::string& service);

	void write(DataBuffer& data, std::function<void(const BufferView&)> onReceive);

	void stop();

protected:

	void connectByHostname();
	void connectEndpoint();

	void postFail(const char* place, const std::error_code& error);

	enum { Disconnected, Connecting, Handshake, Connected } state = Disconnected;
	void handle_resolve(const std::error_code& error, tcp::resolver::results_type results, std::shared_ptr<tcp::resolver> resolver);
	void handle_connect(const std::error_code& error);
	void handle_handshake(const asio::error_code& err);

	void do_write();
 	void handle_write(asio::error_code ec, std::size_t);
 	void handle_receive(asio::error_code, std::size_t received, std::size_t bytes_requested);

	asio::io_context& io;
	asio::ssl::context ctx;
	asio::ssl::stream<tcp::socket> socket;

	struct
	{
		std::string host;
		std::string service;
		tcp::endpoint endpoint;
		bool endpointInitialized = false;
	}
	info;

	std::function<void(const BufferView&)> onReceiveCallback;
	DataBuffer writeBuffer;
	DataBuffer readBuffer;
	DataBuffer readBufferTmp;

	FileLog log;
};

#endif
#pragma once

#include "utils\Network.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>
#include "Bandwidth.h"

class TcpAsyncServer;

class TcpAsyncLimitedStream : public std::enable_shared_from_this<TcpAsyncLimitedStream>, public BandwidthUser
{
	friend class TcpAsyncServer;

public:

	TcpAsyncLimitedStream(asio::io_service& io_service);
	~TcpAsyncLimitedStream();

	void connect(const uint8_t* data, uint16_t port, bool ipv6);

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

	void setBandwidthChannels(BandwidthChannel**, uint32_t count);
	void setBandwidthPriority(int priority);
	void setMinBandwidthRequest(uint32_t size);

protected:

	void connectEndpoint();
	void setAsConnected();

	void postFail(const char* place, const std::error_code& error);

	enum { Disconnected, Connecting, Connected } state = Disconnected;
	void handle_connect(const std::error_code& err);
	void do_close();

	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const std::error_code& error);

	std::vector<char> recv_buffer;
	void handle_receive(const std::error_code& error, std::size_t bytes_transferred);
	void appendData(char* data, size_t size);
	std::mutex receive_mutex;
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

	uint32_t m_quota = 0;
	bool waiting_for_bw = false;
	bool waiting_for_data = false;

	uint32_t wantedTransfer();
	void requestBandwidth(uint32_t size);

	void startReceive();
	bool readAvailableData();

	virtual void assignBandwidth(int amount) override;
	virtual bool isActive() override;

	uint32_t expecting_size = 100;
	int priority = 1;

	BandwidthChannel* bwChannels[2];

	uint32_t lastReceiveSpeed = 0;
	uint32_t lastReceiveTime = 0;
};
#pragma once

#include "Network.h"
#include "Bandwidth.h"
#include <mutex>
#include <future>
#include <memory>
#include <array>
#include <functional>
#include <deque>
#include "Logging.h"

class TcpAsyncServer;

class TcpAsyncStream : public std::enable_shared_from_this<TcpAsyncStream>, public BandwidthUser
{
	friend class TcpAsyncServer;

public:

	TcpAsyncStream(asio::io_context& io_context);
	~TcpAsyncStream();

	void connect(const uint8_t* data, uint16_t port, bool ipv6);
	void connect(const std::string& ip, uint16_t port);
	void init(const std::string& hostname, const std::string& port);

	void close(bool immediate = true);

	void write(DataBuffer&& data);
	void write(const DataBuffer& data);

	std::function<void()> onConnectCallback;
	std::function<size_t(BufferView)> onReceiveCallback;
	std::function<void(int)> onCloseCallback;

	const std::string& getHostname() const;
	const Addr& getAddress() const;

	uint64_t getReceivedDataCount() const;

	void setBandwidthChannels(BandwidthChannel**, uint32_t count);
	void setBandwidthPriority(int priority);
	void setMinBandwidthRequest(uint32_t size);

protected:

	std::mutex callbackMutex;

	void connectByHostname();
	void connectByAddress();
	void setAsConnected();
	void initializeInfo();

	void postFail(const char* place, const std::error_code& error);

	enum { Clear, Connecting, Connected, Disconnected } state = Clear;
	void handle_resolve(const std::error_code& error, tcp::resolver::results_type results, std::shared_ptr<tcp::resolver> resolver);
	void handle_resolver_connect(const std::error_code& err, tcp::resolver::results_type results, uint32_t counter, std::shared_ptr<tcp::resolver> resolver);
	void handle_connect(const std::error_code& err);
	void do_close();

	void do_write(DataBuffer data);
	std::mutex write_msgs_mutex;
	std::deque<DataBuffer> write_msgs;
	void handle_write(const std::error_code& error);

	struct ReadBuffer
	{
		LogWriter* log = nullptr;

		void advanceBuffer(size_t size);
		void consume(size_t size);
		uint8_t* reserve(size_t size);
		size_t reserved();

		DataBuffer data;
		uint64_t receivedCounter = 0;
		size_t pos = 0;
	}
	readBuffer;
	std::mutex receive_mutex;

	void handle_receive(const std::error_code& error, std::size_t bytes_transferred, std::size_t bytes_requested);
	void appendData(size_t size);

	std::mutex socket_mutex;
	tcp::socket socket;

	void checkTimeout(const asio::error_code& error);
	std::unique_ptr<asio::steady_timer> timeoutTimer;

	asio::io_context& io_context;

	struct  
	{
		std::string host;
		Addr address;
		bool addressResolved = false;
	}
	info;

	uint32_t bw_quota = 0;
	bool waiting_for_bw = false;
	bool waiting_for_data = false;

	uint32_t wantedTransfer();
	void requestBandwidth(uint32_t size);

	void startReceive();
	std::error_code readAvailableData();

	void assignBandwidth(int amount) override;
	bool isActive() override;
	std::string name() override;

	uint32_t expecting_size = 100;
	int priority = 1;

	uint32_t bwChannelsCount = 0;
	BandwidthChannel* bwChannels[2];

	uint32_t lastReceiveSpeed = 0;
	uint32_t lastReceiveTime = 0;

	FileLog log;
};

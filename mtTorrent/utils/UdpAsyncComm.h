#pragma once

#include "utils/Network.h"
#include "UdpAsyncReceiver.h"
#include "ServiceThreadpool.h"

using UdpResponseCallback = std::function<bool(UdpRequest, const BufferView&)>;

class UdpAsyncComm
{
public:

	UdpAsyncComm();
	~UdpAsyncComm();

	static UdpAsyncComm& Get();

	void deinit();

	void setBindPort(uint16_t port);

	void listen(UdpPacketCallback received);
	void removeListeners();

	UdpRequest create(const std::string& host, const std::string& port);
	UdpRequest sendMessage(const DataBuffer& data, const std::string& host, const std::string& port, UdpResponseCallback response, bool ipv6 = false, uint32_t timeout = 1, bool anySource = false);
	UdpRequest sendMessage(const DataBuffer& data, const Addr& addr, UdpResponseCallback response);
	void sendMessage(const DataBuffer& data, UdpRequest target, UdpResponseCallback response, uint32_t timeout = 1);
	void sendMessage(const DataBuffer& data, const udp::endpoint& endpoint);

	void removeCallback(UdpRequest target);

private:

	struct ResponseRetryInfo
	{
		~ResponseRetryInfo();

		UdpRequest client;
		uint8_t retries = 0;
		UdpResponseCallback onResponse;
		std::shared_ptr<asio::steady_timer> timeoutTimer;
		uint32_t defaultTimeout = 1;
		bool anySource;
		void reset();
	};

	std::mutex respondingMutex;
	std::mutex responsesMutex;
	std::vector<std::shared_ptr<ResponseRetryInfo>> pendingResponses;
	void addPendingResponse(UdpRequest target, UdpResponseCallback response, uint32_t timeout = 1, bool anySource = false);
	UdpRequest findPendingConnection(UdpRequest);

	void checkTimeout(UdpRequest, const asio::error_code& error);
	void onUdpReceiveBuffers(udp::endpoint&, const std::vector<BufferView>&);
	void onDirectUdpReceive(UdpRequest, const BufferView&);
	bool onUdpReceive(udp::endpoint&, const BufferView&);
	void onUdpClose(UdpRequest);
	UdpPacketCallback onUnhandledReceive;

	void startListening();
	std::shared_ptr<UdpAsyncReceiver> listener;
	uint16_t bindPort = 0;

	ServiceThreadpool pool;
};

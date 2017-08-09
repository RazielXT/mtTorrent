#pragma once

#include "Network.h"
#include "UdpAsyncClient.h"

namespace udp
{
	using Connection = std::shared_ptr<UdpAsyncClient>;

	class AsyncMgr
	{
	public:

		AsyncMgr();

		void listen(uint16_t port, std::function<void(Connection)>);
		void setImplicitPort(uint16_t port);

		using ResponseCallback = std::function<bool(Connection, DataBuffer*)>;
		void sendMessage(DataBuffer& data, std::string& host, std::string& port, ResponseCallback onResult);
		void sendMessage(DataBuffer& data, Addr& addr, ResponseCallback onResult);
		void sendMessage(DataBuffer& data, Connection target, ResponseCallback onResult);

	private:

		struct ConnectionInfo
		{
			Connection client;
			std::function<bool(DataBuffer*)> onResult;
		};
		std::vector<ConnectionInfo> pendingResponses;
		std::mutex responsesMutex;

		uint16_t implicitPort = 0;
	};
}

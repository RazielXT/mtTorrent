#pragma once
#include "utils/ServiceThreadpool.h"
#include "utils/TcpAsyncLimitedStream.h"
#include "Api/Listener.h"

class UpnpPortMapping;

namespace mtt
{
	class IncomingPeersListener : public mttApi::Listener
	{
	public:

		IncomingPeersListener(std::function<void(std::shared_ptr<TcpAsyncLimitedStream>, const uint8_t* hash)> onNewPeer);

		void stop();

		std::string getUpnpReadableInfo();

	protected:

		void createListener();

		std::function<void(std::shared_ptr<TcpAsyncLimitedStream>, const uint8_t* hash)> onNewPeer;

		std::mutex peersMutex;
		std::vector<std::shared_ptr<TcpAsyncLimitedStream>> pendingPeers;

		std::shared_ptr<TcpAsyncServer> listener;
		ServiceThreadpool pool;

		void removePeer(TcpAsyncLimitedStream* s);
		void addPeer(TcpAsyncLimitedStream* s, const uint8_t* hash);


		std::shared_ptr<UpnpPortMapping> upnp;
		bool upnpEnabled;

		struct UsedPorts
		{
			uint16_t tcp;
			uint16_t udp;
		}
		usedPorts;
	};
}
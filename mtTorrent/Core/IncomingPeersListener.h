#pragma once
#include "utils/ServiceThreadpool.h"
#include "utils/TcpAsyncStream.h"

class UpnpPortMapping;

namespace mtt
{
	class IncomingPeersListener
	{
	public:

		IncomingPeersListener(std::function<void(std::shared_ptr<TcpAsyncStream>, const uint8_t* hash)> onNewPeer);

		void stop();

		std::string getUpnpReadableInfo();

	private:

		void createListener();

		std::function<void(std::shared_ptr<TcpAsyncStream>, const uint8_t* hash)> onNewPeer;

		std::mutex peersMutex;
		std::vector<std::shared_ptr<TcpAsyncStream>> pendingPeers;

		std::shared_ptr<TcpAsyncServer> listener;
		ServiceThreadpool pool;

		void removePeer(TcpAsyncStream* s);
		void addPeer(TcpAsyncStream* s, const uint8_t* hash);


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
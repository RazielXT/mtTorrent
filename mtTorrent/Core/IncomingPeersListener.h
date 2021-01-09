#pragma once

#include "utils/ServiceThreadpool.h"
#include "utils/TcpAsyncStream.h"
#include "Api/Listener.h"

class UpnpPortMapping;

namespace mtt
{
	class IncomingPeersListener : public mttApi::Listener
	{
	public:

		IncomingPeersListener(std::function<size_t(std::shared_ptr<TcpAsyncStream>, const BufferView& data, const uint8_t* hash)> onNewPeer);

		void stop();

		std::string getUpnpReadableInfo() const;

	protected:

		void createListener();

		std::function<size_t(std::shared_ptr<TcpAsyncStream>, const BufferView& data, const uint8_t* hash)> onNewPeer;

		std::mutex peersMutex;
		std::vector<std::shared_ptr<TcpAsyncStream>> pendingPeers;

		std::shared_ptr<TcpAsyncServer> listener;
		ServiceThreadpool pool;

		void removePeer(TcpAsyncStream* s);
		size_t addPeer(TcpAsyncStream* s, const BufferView& data, const uint8_t* hash);


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
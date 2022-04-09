#pragma once

#include "utils/ServiceThreadpool.h"
#include "utils/TcpAsyncStream.h"
#include "Api/Listener.h"
#include "PeerStream.h"
#include <set>

class UpnpPortMapping;

namespace mtt
{
	class IncomingPeersListener : public mttApi::Listener, public ProtocolEncryptionListener
	{
	public:

		IncomingPeersListener(std::function<size_t(std::shared_ptr<PeerStream>, const BufferView& data, const uint8_t* hash)> onNewPeer);

		void stop();

		std::string getUpnpReadableInfo() const;

	protected:

		void createTcpListener();
		void createUtpListener();

		size_t readStreamData(BufferSpan data, PeerStream*);

		std::function<size_t(std::shared_ptr<PeerStream>, const BufferView& data, const uint8_t* hash)> onNewPeer;

		std::mutex peersMutex;
		struct PendingPeer
		{
			std::shared_ptr<PeerStream> s;
			std::shared_ptr<ProtocolEncryptionHandshake> peHandshake;
		};
		std::map<void*,PendingPeer> pendingPeers;

		std::shared_ptr<TcpAsyncServer> tcpListener;
		ServiceThreadpool pool;

		void removePeer(void* s);
		size_t startPeer(void* s, const BufferView& data, const uint8_t* hash);
		std::shared_ptr<ProtocolEncryptionHandshake> getPeHandshake(void* s);

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
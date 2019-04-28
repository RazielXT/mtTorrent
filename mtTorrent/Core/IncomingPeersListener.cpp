//auto data = stream->getReceivedData();
//PeerMessage msg(data);
#include "IncomingPeersListener.h"
#include "Configuration.h"
#include "utils/TcpAsyncServer.h"
#include "PeerMessage.h"
#include "utils/UpnpPortMapping.h"

mtt::IncomingPeersListener::IncomingPeersListener(std::function<void(std::shared_ptr<TcpAsyncStream>, const uint8_t* hash)> cb)
{
	onNewPeer = cb;
	pool.start(2);
	listener = std::make_shared<TcpAsyncServer>(pool.io, mtt::config::external.tcpPort, false);
	listener->acceptCallback = [this](std::shared_ptr<TcpAsyncStream> c)
	{
		auto sPtr = c.get();

		c->onCloseCallback = [sPtr, this](int)
		{
			removePeer(sPtr);
		};
		c->onReceiveCallback = [sPtr, this]()
		{
			auto data = sPtr->getReceivedData();
			PeerMessage msg(data);

			if (msg.id == Handshake)
				addPeer(sPtr, msg.handshake.info);
			else if (msg.messageSize == 0)
				removePeer(sPtr);
		};

		std::lock_guard<std::mutex> guard(peersMutex);
		pendingPeers.push_back(c);
	};

	listener->listen();

	upnp = std::make_shared<UpnpPortMapping>(pool.io);
	upnp->mapActiveAdapters(mtt::config::external.tcpPort, UpnpPortMapping::PortType::Tcp);
	upnp->mapActiveAdapters(mtt::config::external.udpPort, UpnpPortMapping::PortType::Udp);
}

void mtt::IncomingPeersListener::stop()
{
	upnp->unmapAllMappedAdapters();

	listener->stop();

	std::lock_guard<std::mutex> guard(peersMutex);
	pendingPeers.clear();
}

void mtt::IncomingPeersListener::removePeer(TcpAsyncStream* s)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto it = pendingPeers.begin(); it != pendingPeers.end(); it++)
	{
		if ((*it).get() == s)
		{
			pendingPeers.erase(it);
			break;
		}
	}
}

void mtt::IncomingPeersListener::addPeer(TcpAsyncStream* s, const uint8_t* hash)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto it = pendingPeers.begin(); it != pendingPeers.end(); it++)
	{
		if ((*it).get() == s)
		{
			auto ptr = *it;
			std::vector<uint8_t> hashData;
			hashData.assign(hash, hash+20);

			pool.io.post([this, ptr, hashData]()
			{
				onNewPeer(ptr, hashData.data());
			});

			pendingPeers.erase(it);
			break;
		}
	}
}

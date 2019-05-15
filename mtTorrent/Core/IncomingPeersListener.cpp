#include "IncomingPeersListener.h"
#include "Configuration.h"
#include "utils/TcpAsyncServer.h"
#include "PeerMessage.h"
#include "utils/UpnpPortMapping.h"

mtt::IncomingPeersListener::IncomingPeersListener(std::function<void(std::shared_ptr<TcpAsyncStream>, const uint8_t* hash)> cb)
{
	onNewPeer = cb;
	pool.start(2);

	createListener();

	upnpEnabled = mtt::config::getExternal().connection.upnpPortMapping;
	upnp = std::make_shared<UpnpPortMapping>(pool.io);

	if (upnpEnabled)
	{
		upnp->mapActiveAdapters(mtt::config::getExternal().connection.tcpPort, UpnpPortMapping::PortType::Tcp);
		upnp->mapActiveAdapters(mtt::config::getExternal().connection.udpPort, UpnpPortMapping::PortType::Udp);
	}

	config::registerOnChangeCallback(config::ValueType::Connection, [this]()
		{
			auto& settings = mtt::config::getExternal().connection;

			if (settings.upnpPortMapping != upnpEnabled)
			{
				if (settings.upnpPortMapping)
				{
					upnp->mapActiveAdapters(settings.tcpPort, UpnpPortMapping::PortType::Tcp);
					upnp->mapActiveAdapters(settings.udpPort, UpnpPortMapping::PortType::Udp);
				}
				else
					upnp->unmapAllMappedAdapters(false);
			}
			else if(upnpEnabled)
			{
				if (usedPorts.udp != settings.udpPort)
				{
					upnp->unmapMappedAdapters(usedPorts.udp, UpnpPortMapping::PortType::Udp, false);
					upnp->mapActiveAdapters(settings.udpPort, UpnpPortMapping::PortType::Udp);
				}
				if (usedPorts.tcp != settings.tcpPort)
				{
					upnp->unmapMappedAdapters(usedPorts.udp, UpnpPortMapping::PortType::Tcp, false);
					upnp->mapActiveAdapters(settings.tcpPort, UpnpPortMapping::PortType::Tcp);
				}
			}
			
			if (usedPorts.tcp != settings.tcpPort)
				createListener();

			usedPorts.tcp = settings.tcpPort;
			usedPorts.udp = settings.udpPort;
			upnpEnabled = settings.upnpPortMapping;
		});

	usedPorts.tcp = mtt::config::getExternal().connection.tcpPort;
	usedPorts.udp = mtt::config::getExternal().connection.udpPort;
}

void mtt::IncomingPeersListener::stop()
{
	upnp->unmapAllMappedAdapters();

	if (listener)
		listener->stop();

	std::lock_guard<std::mutex> guard(peersMutex);
	pendingPeers.clear();
}

void mtt::IncomingPeersListener::createListener()
{
	if (listener)
		listener->stop();

	try
	{
		listener = std::make_shared<TcpAsyncServer>(pool.io, mtt::config::getExternal().connection.tcpPort, false);
	}
	catch (const boost::system::system_error&)
	{
		listener = nullptr;
		return;
	}

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

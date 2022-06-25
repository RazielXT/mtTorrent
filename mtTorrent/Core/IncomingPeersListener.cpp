#include "IncomingPeersListener.h"
#include "Configuration.h"
#include "utils/TcpAsyncServer.h"
#include "PeerMessage.h"
#include "utils/UpnpPortMapping.h"
#include "utils/UdpAsyncComm.h"
#include "Logging.h"

#define PEER_LOG(x) WRITE_GLOBAL_LOG(PeersListener, x)

mtt::IncomingPeersListener::IncomingPeersListener(std::function<size_t(std::shared_ptr<PeerStream>, const BufferView& data, const uint8_t* hash)> cb)
{
	onNewPeer = cb;
	pool.start(2);

	createTcpListener();
	createUtpListener();

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
				createTcpListener();

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

	if (tcpListener)
	{
		tcpListener->stop();
		tcpListener = nullptr;
	}

	utp::Manager::get().setConnectionCallback(nullptr);

	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto [id,peer] : pendingPeers)
	{
		peer.s->onCloseCallback = [](int) {};
		peer.s->onReceiveCallback = [](BufferSpan) { return 0; };
	}
	pendingPeers.clear();
}

std::string mtt::IncomingPeersListener::getUpnpReadableInfo() const
{
	return upnp->getReadableInfo();
}

void mtt::IncomingPeersListener::createTcpListener()
{
	if (tcpListener)
		tcpListener->stop();

	try
	{
		tcpListener = std::make_shared<TcpAsyncServer>(pool.io, mtt::config::getExternal().connection.tcpPort, false);
	}
	catch (const std::system_error&)
	{
		tcpListener = nullptr;
		return;
	}

	tcpListener->acceptCallback = [this](std::shared_ptr<TcpAsyncStream> s)
	{
		if (!mtt::config::getExternal().connection.enableTcpIn)
		{
			s->close(false);
			return;
		}

		auto stream = std::make_shared<PeerStream>(pool.io);
		auto sPtr = stream.get();
		stream->fromStream(s);
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			pendingPeers[sPtr].s = stream;
		}

		stream->onCloseCallback = [sPtr, this](int)
		{
			removePeer(sPtr);
		};
		stream->onReceiveCallback = [sPtr, this](BufferSpan data) -> size_t
		{
			return readStreamData(data, sPtr);
		};
	};

	tcpListener->listen();
}

void mtt::IncomingPeersListener::createUtpListener()
{
	auto& utp = utp::Manager::get();

	utp.setConnectionCallback([this](utp::StreamPtr s)
	{
		auto stream = std::make_shared<PeerStream>(pool.io);
		auto sPtr = stream.get();
		stream->fromStream(s);
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			pendingPeers[sPtr].s = stream;
		}
		PEER_LOG("utp open " << sPtr->getAddress());

		stream->onCloseCallback = [sPtr, this](int)
		{
			PEER_LOG("utp close " << sPtr->getAddress());
			removePeer(sPtr);
		};
		stream->onReceiveCallback = [sPtr, this](BufferSpan data) -> size_t
		{
			return readStreamData(data, sPtr);
		};
	});
}

size_t mtt::IncomingPeersListener::readStreamData(BufferSpan data, PeerStream* sPtr)
{
	PEER_LOG("Received " << data.size << " bytes from " << sPtr->getAddress());

	if (data.size < 20)
		return 0;

	// standard handshake
	if (PeerMessage::startsAsHandshake(data))
	{
		PeerMessage msg(data);

		size_t sz = startPeer(sPtr, data, msg.handshake.info);
		if (sz == 0)
		{
			sPtr->close();
		}
		return sz;
	}

	// protocol encryption handshake
	if (auto peHandshake = getPeHandshake(sPtr))
	{
		DataBuffer response;
		size_t sz = peHandshake->readRemoteDataAsListener(data, response, *this);
		PEER_LOG("peHandshake read " << sz);

		if (!response.empty())
		{
			PEER_LOG("peHandshake response size " << response.size());
			sPtr->write(response);
		}

		if (peHandshake->established())
		{
			PEER_LOG("peHandshake established");
			startPeer(sPtr, peHandshake->getInitialBtMessage(), peHandshake->getTorrentHash());
		}
		else if (peHandshake->failed())
		{
			PEER_LOG("peHandshake failed");
			sPtr->close();
		}

		return sz;
	}

	return 0;
}

void mtt::IncomingPeersListener::removePeer(void* s)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = pendingPeers.find(s);

	if (it != pendingPeers.end())
		pendingPeers.erase(it);
}

size_t mtt::IncomingPeersListener::startPeer(void* s, const BufferView& data, const uint8_t* hash)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = pendingPeers.find(s);

	if (it != pendingPeers.end())
	{
		auto peer = it->second;
		pendingPeers.erase(it);

		if (peer.peHandshake)
			peer.s->addEncryption(std::move(peer.peHandshake->pe));

		return onNewPeer(peer.s, data, hash);
	}

	return 0;
}

std::shared_ptr<ProtocolEncryptionHandshake> mtt::IncomingPeersListener::getPeHandshake(void* s)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = pendingPeers.find(s);

	if (it != pendingPeers.end())
	{
		if (!it->second.peHandshake)
			it->second.peHandshake = std::make_shared<ProtocolEncryptionHandshake>();

		return it->second.peHandshake;
	}

	return nullptr;
}

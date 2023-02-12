#include "Peers.h"
#include "Torrent.h"
#include "PeerCommunication.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "Uploader.h"
#include <fstream>

enum class LogEvent : uint8_t { Connect, RemoteConnect, Remove, ConnectPeers };

#define DIAGNOSTICS(eventType, x) WRITE_DIAGNOSTIC_LOG((char)LogEvent::eventType << x)

#define PEERS_LOG(x) WRITE_LOG(x)

mtt::Peers::Peers(Torrent& t) : torrent(t), trackers(t), dht(*this, t), listener(&nolistener)
{
	pex.info.hostname = "PEX";
	remoteInfo.hostname = "Remote";
	reloadTorrentInfo();

	CREATE_NAMED_LOG(Peers, torrent.name() + "_peers");
}

void mtt::Peers::start(IPeerListener* listener)
{
	setTargetListener(listener);

	trackers.start([this](Status s, const AnnounceResponse* r, const Tracker& t)
	{
		if (r)
		{
			PEERS_LOG(t.info.hostname << " returned " << r->peers.size());
			for (auto& a : r->peers)
			{
				PEERS_LOG(t.info.hostname << " " << a.toString());
			}
			updateKnownPeers(r->peers, PeerSource::Tracker);
		}
	}
	);

	refreshTimer = ScheduledTimer::create(torrent.service.io, [this] { update(); return ScheduledTimer::DurationSeconds(1); });
	refreshTimer->schedule(ScheduledTimer::DurationSeconds(1));

	dht.start();

	pex.info.state = TrackerState::Connected;
	remoteInfo.state = TrackerState::Connected;

	if (connecting)
		torrent.service.post([this]() { connectNext(); });
}

void mtt::Peers::stop()
{
	trackers.stop();
	dht.stop();

	{
		setTargetListener(nullptr);
		stopConnecting();

		std::lock_guard<std::mutex> guard(peersMutex);

		int i = 0;
		for (const auto& c : activeConnections)
		{	
			if (c.comm)
			{
				auto& peer = knownPeers[c.idx];
				peer.state = KnownPeer::State::Disconnected;
				PEERS_LOG("Stop " << peer.address);

				c.comm->close();
			}

			i++;
		}

		activeConnections.clear();
	}

	if (refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	pex.info.state = TrackerState::Clear;
	remoteInfo.state = TrackerState::Clear;
	remoteInfo.peers = 0;
}

void mtt::Peers::startConnecting()
{
	if (!connecting)
	{
		connecting = true;

		if (!stopped())
			torrent.service.post([this]() { connectNext(); });
	}
}

void mtt::Peers::stopConnecting()
{
	connecting = false;
}

void mtt::Peers::connectNext()
{
	PEERS_LOG("connectNext");

	std::lock_guard<std::mutex> guard(peersMutex);

	if (!connecting || stopped() || mtt::config::getExternal().connection.maxTorrentConnections <= activeConnections.size())
		return;

	auto count = mtt::config::getExternal().connection.maxTorrentConnections - (uint32_t)activeConnections.size();
	uint32_t origCount = count;

	auto currentTime = mtt::CurrentTimestamp();
	const mtt::Timestamp MinTimeToReconnect = 30;

	for (size_t i = 0; i < knownPeers.size(); i++)
	{
		if (count == 0)
			break;

		auto idx = (uint32_t)currentConnectIdx;

		auto& p = knownPeers[idx];
		if (p.state == KnownPeer::State::Disconnected && !p.unwanted && (p.lastConnectionTime == 0 || p.lastConnectionTime + MinTimeToReconnect < currentTime))
		{
			p.state = KnownPeer::State::Connecting;
			p.lastConnectionTime = currentTime;
			p.connectionAttempts++;
			connect(p.address, idx);
			count--;
		}

		currentConnectIdx++;

		if (currentConnectIdx == knownPeers.size())
			currentConnectIdx = 0;
	}

	PEERS_LOG("connected " << (origCount - count));
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::removePeer(PeerCommunication* p, KnownPeer& info)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto it = activeConnections.begin(); it != activeConnections.end(); it++)
	{
		if (it->comm.get() == p)
		{
			auto ptr = it->comm;
			auto& peer = knownPeers[it->idx];
			peer.state = KnownPeer::State::Disconnected;
			peer.lastConnectionTime = mtt::CurrentTimestamp();

			activeConnections.erase(it);
			PEERS_LOG("Remove" << peer.address);
			info = peer;

			return ptr;
		}
	}

	return nullptr;
}

mtt::Status mtt::Peers::connect(const Addr& addr)
{
	if (!addr.valid())
		return mtt::Status::E_InvalidInput;
	if (stopped())
		return mtt::Status::I_Stopped;

	{
		std::lock_guard<std::mutex> guard(peersMutex);
		if (getActivePeer(addr))
			return mtt::Status::I_AlreadyExists;

		auto it = std::find(knownPeers.begin(), knownPeers.end(), addr);
		uint32_t idx = 0;

		if (it == knownPeers.end())
		{
			knownPeers.push_back({ addr, PeerSource::Manual });
			idx = (uint32_t)knownPeers.size() - 1;

			PEERS_LOG("New manual peer " << addr);
		}
		else
			idx = (uint32_t)std::distance(knownPeers.begin(), it);

		KnownPeer& peer = knownPeers[idx];
		peer.state = KnownPeer::State::Connecting;
		peer.lastConnectionTime = mtt::CurrentTimestamp();
		peer.connectionAttempts++;
		connect(addr, idx);
	}

	return mtt::Status::Success;
}

size_t mtt::Peers::add(std::shared_ptr<PeerStream> stream, const BufferView& data)
{
	if (!torrent.isActive())
		return 0;

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent.infoFile.info, *this, torrent.service.io);

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		KnownPeer p;
		p.address = stream->getAddress();
		p.source = PeerSource::Remote;
		p.state = KnownPeer::State::Connected;
		knownPeers.push_back(p);

		peer.idx = (uint32_t)knownPeers.size() - 1;
		activeConnections.push_back(peer);
		DIAGNOSTICS(RemoteConnect, p.address);

		remoteInfo.peers++;
	}

	return peer.comm->fromStream(stream, data);
}

bool mtt::Peers::disconnect(PeerCommunication* p, bool unwanted)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (const auto& activeConnection : activeConnections)
	{
		if (activeConnection.comm.get() == p)
		{
			PEERS_LOG("Disconnect " << p->getStream()->getAddress() << " unwanted " << unwanted);
			if (unwanted)
				knownPeers[activeConnection.idx].unwanted = true;

			p->close();
			return true;
		}
	}

	return false;
}

std::vector<mtt::TrackerInfo> mtt::Peers::getSourcesInfo()
{
	torrent.loadFileInfo();

	std::vector<mtt::TrackerInfo> out;
	auto tr = trackers.getTrackers();
	out.reserve(tr.size() + 2);

	for (auto& t : tr)
	{
		out.push_back(t.second ? t.second->info : mtt::TrackerInfo{});
		out.back().hostname = t.first;
	}

	out.push_back(pex.info);

	if (mtt::config::getExternal().dht.enabled)
		out.push_back(dht.info);

	out.push_back(remoteInfo);

	return out;
}

void mtt::Peers::refreshSource(const std::string& name)
{
	if (stopped())
		return;

	if (auto t = trackers.getTrackerByAddr(name))
		t->announce();
	else if (name == "DHT")
		dht.findPeers();
}

uint32_t mtt::Peers::connectedCount() const
{
	return (uint32_t)activeConnections.size();
}

uint32_t mtt::Peers::receivedCount() const
{
	return (uint32_t)knownPeers.size();
}

std::vector<std::shared_ptr<mtt::PeerCommunication>> mtt::Peers::getActivePeers() const
{
	std::vector<std::shared_ptr<mtt::PeerCommunication>> out;
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& peer : activeConnections)
	{
		out.push_back(peer.comm);
	}

	return out;
}

void mtt::Peers::reloadTorrentInfo()
{
	trackers.addTrackers(torrent.infoFile.announceList);

	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& peer : activeConnections)
	{
		if (peer.comm->isEstablished())
			peer.comm->info.pieces.resize(torrent.infoFile.info.pieces.size());
	}
}

uint32_t mtt::Peers::updateKnownPeers(const std::vector<Addr>& peers, PeerSource source, const uint8_t* flags)
{
	if (peers.empty())
		return 0;

	std::vector<uint32_t> newPeers;
	newPeers.reserve(10);

	uint32_t updatedPeers = 0;
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		for (uint32_t i = 0; i < peers.size(); i++)
		{
			auto it = std::find(knownPeers.begin(), knownPeers.end(), peers[i]);

			if (it == knownPeers.end())
				newPeers.push_back(i);
			else if (flags)
			{
				if (it->pexFlags == 0)
					updatedPeers++;

				it->pexFlags |= flags[i];
			}
		}

		if (newPeers.empty())
			return updatedPeers;

		KnownPeer* addedPeersPtr;

		if ((source == PeerSource::Pex || source == PeerSource::Dht) && currentConnectIdx != 0)
		{
			knownPeers.insert(knownPeers.begin() + currentConnectIdx, newPeers.size(), {});
			addedPeersPtr = &knownPeers[currentConnectIdx];

			for (auto& conn : activeConnections)
				if (conn.idx >= currentConnectIdx)
					conn.idx += (uint32_t)newPeers.size();
		}
		else
		{
			if (currentConnectIdx == 0)
				currentConnectIdx = knownPeers.size();

			knownPeers.resize(knownPeers.size() + newPeers.size());
			addedPeersPtr = &knownPeers[knownPeers.size() - newPeers.size()];
		}

		PEERS_LOG("New peers " << newPeers.size() << " source " << (int)source << " current pos " << currentConnectIdx << " all peers " << knownPeers.size());

		for (auto idx : newPeers)
		{
			addedPeersPtr->address = peers[idx];
			PEERS_LOG("New peer " << addedPeersPtr->address);
			addedPeersPtr->source = source;
			if (flags)
				addedPeersPtr->pexFlags = flags[idx];
			addedPeersPtr++;
		}
	}

	connectNext();

	return (uint32_t)newPeers.size() + updatedPeers;
}

uint32_t mtt::Peers::updateKnownPeers(const ext::PeerExchange::Message& pex)
{
	auto accepted = updateKnownPeers(pex.addedPeers, PeerSource::Pex, pex.addedFlags.data);
	accepted += updateKnownPeers(pex.added6Peers, PeerSource::Pex, pex.added6Flags.data);

	return accepted;
}

void mtt::Peers::connect(const Addr& address, uint32_t idx)
{
	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent.infoFile.info, *this, torrent.service.io);
	peer.comm->connect(address);
	peer.idx = idx;
	activeConnections.emplace_back(std::move(peer));

	PEERS_LOG("connect " << address);
}

void mtt::Peers::update()
{
	dht.update();

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		std::vector<PeerCommunication*> peers;
		peers.reserve(activeConnections.size());
		for (const auto& c : activeConnections)
		{
			peers.push_back(c.comm.get());
		}

		pex.local.update(peers);

		for (const auto& c : activeConnections)
		{
			c.comm->ext.pex.update(pex.local);
		}
	}
}

bool mtt::Peers::stopped() const
{
	return refreshTimer == nullptr;
}

mtt::Peers::ActivePeer* mtt::Peers::getActivePeer(const Addr& addr)
{
	for (auto& connection : activeConnections)
	{
		if (connection.comm->getStream()->getAddress() == addr)
			return &connection;
	}

	return nullptr;
}

mtt::Peers::ActivePeer* mtt::Peers::getActivePeer(PeerCommunication* p)
{
	for (auto& connection : activeConnections)
	{
		if (connection.comm.get() == p)
			return &connection;
	}

	return nullptr;
}

mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(PeerCommunication* p)
{
	if (auto peer = getActivePeer(p))
		return &knownPeers[peer->idx];

	return nullptr;
}


bool mtt::Peers::KnownPeer::operator==(const Addr& r)
{
	return address == r;
}

mtt::Peers::DhtSource::DhtSource(Peers& p, Torrent& t) : peers(p), torrent(t)
{
	info.hostname = "DHT";
	info.announceInterval = mtt::config::getInternal().dht.peersCheckInterval;
}

void mtt::Peers::DhtSource::start()
{
	if (info.state != TrackerState::Clear)
		return;

	info.state = TrackerState::Connected;

	cfgCallbackId = mtt::config::registerOnChangeCallback(config::ValueType::Dht, [this]() { update(); });
}

void mtt::Peers::DhtSource::stop()
{
	dht::Communication::get().stopFindingPeers(torrent.hash());

	mtt::config::unregisterOnChangeCallback(cfgCallbackId);
	info.nextAnnounce = 0;
	info.state = TrackerState::Clear;
}

void mtt::Peers::DhtSource::update()
{
	const auto currentTime = mtt::CurrentTimestamp();

	if (info.nextAnnounce <= currentTime && info.state == TrackerState::Connected)
	{
		if (torrent.selectionFinished())
			info.nextAnnounce = currentTime + info.announceInterval;
		else
			findPeers();
	}
}

void mtt::Peers::DhtSource::findPeers()
{
	if (mtt::config::getExternal().dht.enabled && info.state != TrackerState::Announcing)
	{
		info.state = TrackerState::Announcing;
		info.nextAnnounce = 0;
		dht::Communication::get().findPeers(torrent.hash(), this);
	}
}

uint32_t mtt::Peers::DhtSource::dhtFoundPeers(const uint8_t* hash, const std::vector<Addr>& values)
{
	info.peers += peers.updateKnownPeers(values, PeerSource::Dht);
	return info.peers;
}

void mtt::Peers::DhtSource::dhtFindingPeersFinished(const uint8_t* hash, uint32_t count)
{
	const auto currentTime = mtt::CurrentTimestamp();
	info.lastAnnounce = currentTime;
	info.nextAnnounce = currentTime + info.announceInterval;
	info.state = TrackerState::Connected;
}

void mtt::Peers::handshakeFinished(mtt::PeerCommunication* p)
{
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		if (auto peer = getKnownPeer(p))
			peer->state = KnownPeer::State::Connected;
	}

	listener.load()->handshakeFinished(p);
}

void mtt::Peers::connectionClosed(mtt::PeerCommunication* p, int code)
{
	KnownPeer info;
	auto ptr = removePeer(p, info);

	if (ptr)
		evaluatePossibleHolepunch(p, info);

	connectNext();

	listener.load()->connectionClosed(p, code);
}

void mtt::Peers::messageReceived(mtt::PeerCommunication* p, mtt::PeerMessage& m)
{
	if (m.id == PeerMessage::Port && m.port)
	{
		if (mtt::config::getExternal().dht.enabled)
		{
			Addr addr = p->getStream()->getAddress();
			addr.port = m.port;
			dht::Communication::get().pingNode(addr);
		}
	}

	listener.load()->messageReceived(p, m);
}

void mtt::Peers::setTargetListener(mtt::IPeerListener* t)
{
	if (t)
	{
		listener = t;

		std::lock_guard<std::mutex> guard(peersMutex);
		for (const auto& c : activeConnections)
		{
			if (c.comm->isEstablished())
			{
				t->handshakeFinished(c.comm.get());

				if (c.comm->ext.enabled())
					t->extendedHandshakeFinished(c.comm.get(), {});
			}

			c.comm->sendKeepAlive();
		}
	}
	else
		listener = &nolistener;
}

void mtt::Peers::evaluatePossibleHolepunch(PeerCommunication* p, const KnownPeer& info)
{
	if (!p->getStream()->wasConnected() && !p->getStream()->usedHolepunch() && (info.pexFlags & (ext::PeerExchange::SupportsUth | ext::PeerExchange::SupportsUtp)))
	{
		std::shared_ptr<PeerCommunication> negotiator;
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			for (const auto& active : activeConnections)
			{
				if (active.comm->ext.holepunch.enabled() && active.comm->ext.pex.wasConnected(info.address))
				{
					negotiator = active.comm;
					break;
				}
			}
		}

		if (negotiator)
		{
			std::lock_guard<std::mutex> guard(holepunchMutex);

			HolepunchState s;
			s.target = info.address;
			s.negotiator = negotiator.get();
			s.time = mtt::CurrentTimestamp();
			holepunchStates.push_back(s);

			PEERS_LOG("HolepunchMessage sendRendezvous " << info.address << negotiator->getStream()->getAddress());
			negotiator->ext.holepunch.sendRendezvous(info.address);
		}
	}
}

void mtt::Peers::handleHolepunchMessage(PeerCommunication* p, const ext::UtHolepunch::Message& msg)
{
	PEERS_LOG("HolepunchMessage " << msg.id);

	if (msg.id == ext::UtHolepunch::Rendezvous)
	{
		std::shared_ptr<PeerCommunication> target;
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			if (auto active = getActivePeer(msg.addr))
				target = active->comm;
		}

		if (target)
		{
			if (target->getStream()->isUtp() && target->ext.holepunch.enabled())
			{
				target->ext.holepunch.sendConnect(p->getStream()->getAddress());
				p->ext.holepunch.sendConnect(msg.addr);
			}
		}
		else
			p->ext.holepunch.sendError(msg.addr, ext::UtHolepunch::E_NotConnected);
	}
	else if (msg.id == ext::UtHolepunch::Connect)
	{
		bool wanted = false;
		{
			std::lock_guard<std::mutex> guard(holepunchMutex);
			for (auto it = holepunchStates.begin(); it != holepunchStates.end(); it++)
			{
				if (it->negotiator == p && it->target == msg.addr)
				{
					wanted = true;
					holepunchStates.erase(it);
					break;
				}
			}
		}
		if (wanted)
			connect(msg.addr);
	}
	else if (msg.id == ext::UtHolepunch::Error)
	{
		PEERS_LOG("HolepunchMessageErr " << msg.e);

		std::lock_guard<std::mutex> guard(holepunchMutex);
		for (auto it = holepunchStates.begin(); it != holepunchStates.end(); it++)
		{
			if (it->negotiator == p && it->target == msg.addr)
			{
				holepunchStates.erase(it);
				break;
			}
		}
	}
}

void mtt::Peers::extendedHandshakeFinished(PeerCommunication* p, const ext::Handshake& h)
{
	listener.load()->extendedHandshakeFinished(p, h);
}

void mtt::Peers::extendedMessageReceived(PeerCommunication* p, ext::Type t, const BufferView& data)
{
	if (t == ext::Type::UtHolepunch)
	{
		ext::UtHolepunch::Message msg;

		if (ext::UtHolepunch::Load(data, msg))
		{
			handleHolepunchMessage(p, msg);
		}
	}
	else if (t == ext::Type::Pex)
	{
		ext::PeerExchange::Message msg;

		if (p->ext.pex.load(data, msg))
		{
			pex.info.peers += updateKnownPeers(msg);
		}
	}

	listener.load()->extendedMessageReceived(p, t, data);
}

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

mtt::Peers::Peers(Torrent& t) : torrent(t), trackers(t), dht(*this, t)
{
	pex.info.hostname = "PEX";
	remoteInfo.hostname = "Remote";
	trackers.addTrackers(torrent.infoFile.announceList);

	CREATE_NAMED_LOG(Peers, torrent.name() + "_peers");
}

void mtt::Peers::start(PeersUpdateCallback onPeersUpdated, IPeerListener* listener)
{
	updateCallback = onPeersUpdated;
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

	refreshTimer = ScheduledTimer::create(torrent.service.io, [this] { update(); });
	refreshTimer->schedule(1);

	dht.start();

	std::lock_guard<std::mutex> guard(peersMutex);
	for (const auto& c : activeConnections)
	{
		if (c.comm->isEstablished())
		{
			listener->handshakeFinished(c.comm.get());

			if (c.comm->ext.enabled())
				listener->extendedHandshakeFinished(c.comm.get(), {});
		}

		c.comm->sendKeepAlive();
	}

	pex.info.state = TrackerState::Connected;
	remoteInfo.state = TrackerState::Connected;
}

void mtt::Peers::stop()
{
	trackers.stop();
	dht.stop();

	{
		setTargetListener(nullptr);

		std::lock_guard<std::mutex> guard(peersMutex);

		int i = 0;
		for (const auto& c : activeConnections)
		{	
			if (c.comm)
			{
				auto& peer = knownPeers[c.idx];
				peer.lastQuality = c.comm->isEstablished() ? Peers::PeerQuality::Closed : Peers::PeerQuality::Unknown;

				DIAGNOSTICS(Remove, peer.address << (char)peer.lastQuality);

				c.comm->close();
			}

			i++;
		}

		activeConnections.clear();
	}

	if (refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	updateCallback = nullptr;
	pex.info.state = TrackerState::Clear;
	remoteInfo.state = TrackerState::Clear;
	remoteInfo.peers = 0;
}

void mtt::Peers::connectNext(uint32_t count)
{
	PEERS_LOG("connectNext " << count);

	std::lock_guard<std::mutex> guard(peersMutex);

	if (mtt::config::getExternal().connection.maxTorrentConnections <= activeConnections.size())
		return;

	count = std::min(count, mtt::config::getExternal().connection.maxTorrentConnections - (uint32_t)activeConnections.size());

	const auto currentTime = mtt::CurrentTimestamp();
	uint32_t leastConnectionAttempts = 0;

	uint32_t origCount = count;

	for (size_t i = 0; i < knownPeers.size(); i++)
	{
		if (count == 0)
			break;

		auto&p = knownPeers[i];
		if (p.lastQuality == PeerQuality::Unknown)
		{
			connect((uint32_t)i);
			count--;
		}
		else if (p.lastQuality < PeerQuality::Connecting)
			leastConnectionAttempts = p.connectionAttempts;
	}

	if (count > 0)
	{
		for (size_t i = 0; i < knownPeers.size(); i++)
		{
			if (count == 0)
				break;

			auto & p = knownPeers[i];
			if (p.lastQuality < PeerQuality::Connecting && p.connectionAttempts <= leastConnectionAttempts && p.lastConnectionTime + 30 < currentTime)
			{
				connect((uint32_t)i);
				p.lastConnectionTime = currentTime;
				p.connectionAttempts++;
				count--;
			}
		}
	}

	PEERS_LOG("connected " << (origCount - count));
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::disconnect(PeerCommunication* p, KnownPeer* info)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto it = activeConnections.begin(); it != activeConnections.end(); it++)
	{
		if (it->comm.get() == p)
		{
			auto ptr = it->comm;
			auto& peer = knownPeers[it->idx];

			if (!p->isEstablished())
				peer.lastQuality = Peers::PeerQuality::Offline;
			else
			{
				p->close();
				peer.lastQuality = Peers::PeerQuality::Closed;
			}

			activeConnections.erase(it);
			DIAGNOSTICS(Remove, peer.address << (char)peer.lastQuality);
			if (info)
				*info = peer;

			return ptr;
		}
	}

	return nullptr;
}

void mtt::Peers::connect(const Addr& addr)
{
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		if (getActivePeer(addr))
			return;
	}

	if (addr.port != 0)
	{
		auto idx = updateKnownPeers(addr, PeerSource::Manual);

		std::lock_guard<std::mutex> guard(peersMutex);
		connect(idx);
	}
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
		knownPeers.push_back(p);

		peer.idx = (uint32_t)knownPeers.size() - 1;
		activeConnections.push_back(peer);
		DIAGNOSTICS(RemoteConnect, p.address);

		remoteInfo.peers++;
	}

	return peer.comm->fromStream(stream, data);
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::disconnect(PeerCommunication* p)
{
	return disconnect(p, nullptr);
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
	if (trackers.getTrackersCount() == 0)
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
			return 0;

		KnownPeer* addedPeersPtr;

		if (source == PeerSource::Pex || source == PeerSource::Dht)
		{
			knownPeers.insert(knownPeers.begin(), newPeers.size(), KnownPeer());
			addedPeersPtr = &knownPeers[0];

			for (auto& conn : activeConnections)
			{
				conn.idx += (uint32_t)newPeers.size();
			}
		}
		else
		{
			knownPeers.resize(knownPeers.size() + newPeers.size());
			addedPeersPtr = &knownPeers[knownPeers.size() - newPeers.size()];
		}

		for (uint32_t i : newPeers)
		{
			addedPeersPtr->address = peers[i];
			PEERS_LOG("New peer " << addedPeersPtr->address.toString() << " source " << (int)source);
			addedPeersPtr->source = source;
			if (flags)
				addedPeersPtr->pexFlags = flags[i];
			addedPeersPtr++;
		}
	}

	if (updateCallback)
		updateCallback(mtt::Status::Success, source);

	return (uint32_t)newPeers.size() + updatedPeers;
}

uint32_t mtt::Peers::updateKnownPeers(const ext::PeerExchange::Message& pex)
{
	auto accepted = updateKnownPeers(pex.addedPeers, PeerSource::Pex, pex.addedFlags.data);
	accepted += updateKnownPeers(pex.added6Peers, PeerSource::Pex, pex.added6Flags.data);

	return accepted;
}

uint32_t mtt::Peers::updateKnownPeers(const Addr& addr, PeerSource source)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = std::find(knownPeers.begin(), knownPeers.end(), addr);

	if (it == knownPeers.end())
	{
		KnownPeer peer;
		peer.address = addr;
		peer.source = source;
		knownPeers.push_back(peer);
		PEERS_LOG("New peer " << addr.toString());

		return (uint32_t)knownPeers.size() - 1;
	}

	it->lastQuality = PeerQuality::Unknown;
		
	return (uint32_t)std::distance(knownPeers.begin(), it);
}

void mtt::Peers::connect(uint32_t idx)
{
	auto& knownPeer = knownPeers[idx];
	if (knownPeer.lastQuality == PeerQuality::Unknown)
		knownPeer.lastQuality = PeerQuality::Connecting;

	knownPeer.lastConnectionTime = mtt::CurrentTimestamp();
	knownPeer.connectionAttempts++;

	PEERS_LOG("connect " << knownPeer.address.toString());

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent.infoFile.info, *this, torrent.service.io);
	peer.comm->connect(knownPeer.address);
	peer.idx = idx;
	activeConnections.emplace_back(std::move(peer));

	DIAGNOSTICS(Connect, knownPeer.address);
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

	refreshTimer->schedule(1);
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
			peer->lastQuality = Peers::PeerQuality::Normal;
	}

	std::lock_guard<std::mutex> guard(listenerMtx);
	if (listener)
		listener->handshakeFinished(p);
}

void mtt::Peers::connectionClosed(mtt::PeerCommunication* p, int code)
{
	KnownPeer info;
	auto ptr = disconnect(p, &info);

	evaluatePossibleHolepunch(p, info);

	std::lock_guard<std::mutex> guard(listenerMtx);
	if (listener)
		listener->connectionClosed(p, code);
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

	std::lock_guard<std::mutex> guard(listenerMtx);
	if (listener)
		listener->messageReceived(p, m);
}

void mtt::Peers::setTargetListener(mtt::IPeerListener* t)
{
	std::lock_guard<std::mutex> guard(listenerMtx);
	listener = t;
}

void mtt::Peers::evaluatePossibleHolepunch(PeerCommunication* p, const KnownPeer& info)
{
	if (!p->getStream()->wasConnected() && !p->getStream()->usedHolepunch() && (info.pexFlags & (ext::PeerExchange::SupportsUth | ext::PeerExchange::SupportsUtp)))
	{
		std::shared_ptr<PeerCommunication> negotiator;
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			for (const auto& p : activeConnections)
			{
				if (p.comm->ext.holepunch.enabled() && p.comm->ext.pex.wasConnected(info.address))
				{
					negotiator = p.comm;
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
	std::lock_guard<std::mutex> guard(listenerMtx);
	if (listener)
		listener->extendedHandshakeFinished(p, h);
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
			updateKnownPeers(msg);
		}
	}

	std::lock_guard<std::mutex> guard(listenerMtx);
	if (listener)
		listener->extendedMessageReceived(p, t, data);
}

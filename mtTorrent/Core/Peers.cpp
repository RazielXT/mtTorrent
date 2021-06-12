#include "Peers.h"
#include "Torrent.h"
#include "PeerCommunication.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "Uploader.h"
#include <fstream>

#define PEERS_LOG(x) WRITE_LOG("Peers", x)

mtt::Peers::Peers(TorrentPtr t) : torrent(t), trackers(t), dht(*this, t)
{
	pexInfo.hostname = "PEX";
	remoteInfo.hostname = "Remote";
	trackers.addTrackers(t->infoFile.announceList);
	peersListener = std::make_shared<PeersListener>(this);
}

void mtt::Peers::start(PeersUpdateCallback onPeersUpdated, IPeerListener* listener)
{
	updateCallback = onPeersUpdated;
	peersListener->setTarget(listener);
	peersListener->setParent(this);
	trackers.start([this](Status s, const AnnounceResponse* r, Tracker* t)
	{
		if (r)
		{
			PEERS_LOG(t->info.hostname << " returned " << r->peers.size());
			for (auto& a : r->peers)
			{
				PEERS_LOG(t->info.hostname << " " << a.toString());
			}
			updateKnownPeers(r->peers, PeerSource::Tracker);
		}
	}
	);

	dht.start();

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto c : activeConnections)
	{
		if (c.comm->isEstablished())
		{
			listener->handshakeFinished(c.comm.get());

			if(c.comm->ext.state.enabled)
				listener->extHandshakeFinished(c.comm.get());
		}

		c.comm->sendKeepAlive();
	}

	pexInfo.state = TrackerState::Connected;
	remoteInfo.state = TrackerState::Connected;
}

void mtt::Peers::stop()
{
	saveLogEvents();

	trackers.stop();
	dht.stop();

	{
		peersListener->setTarget(nullptr);
		peersListener->setParent(nullptr);

		std::lock_guard<std::mutex> guard(peersMutex);

		int i = 0;
		for (auto c : activeConnections)
		{	
			if (c.comm)
			{
				auto& peer = knownPeers[c.idx];
				peer.lastQuality = c.comm->isEstablished() ? Peers::PeerQuality::Closed : Peers::PeerQuality::Unknown;

				addLogEvent(Remove, peer.address, (char)peer.lastQuality);

				c.comm->close();
			}

			i++;
		}

		activeConnections.clear();
	}

	updateCallback = nullptr;
	pexInfo.state = TrackerState::Clear;
	remoteInfo.state = TrackerState::Clear;
	remoteInfo.peers = 0;
}

void mtt::Peers::connectNext(uint32_t count)
{
	PEERS_LOG("connectNext " << count);

	std::lock_guard<std::mutex> guard(peersMutex);
	count = std::min(count, mtt::config::getExternal().connection.maxTorrentConnections - (uint32_t)activeConnections.size());

	auto currentTime = (uint32_t)::time(0);
	uint32_t leastConnectionAttempts = 0;

	uint32_t origCount = count;

	for (size_t i = 0; i < knownPeers.size(); i++)
	{
		if(count == 0)
			break;

		auto&p = knownPeers[i];
		if (p.lastQuality == PeerQuality::Unknown)
		{
			connect((uint32_t)i);
			p.lastConnectionTime = currentTime;
			p.connectionAttempts++;
			count--;
		}
		else if(p.lastQuality < PeerQuality::Connecting)
			leastConnectionAttempts = p.connectionAttempts;
	}

#ifdef PEER_DIAGNOSTICS
	if (origCount > 0)
	{
		std::lock_guard<std::mutex> guard(logmtx);

		logevents.push_back({ ConnectPeers, 0,  (uint16_t)count, origCount, clock() });
	}
#endif

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

#ifdef PEER_DIAGNOSTICS
		if (origCount > 0)
		{
			std::lock_guard<std::mutex> guard(logmtx);

			logevents.push_back({ ConnectPeers, 0,  (uint16_t)count, origCount, clock() });
		}
#endif
	}

	PEERS_LOG("connected " << (origCount - count));
}

void mtt::Peers::connect(const Addr& addr)
{
	auto peer = getActivePeer(addr);

	if (!peer && addr.port != 0)
	{
		auto idx = updateKnownPeers(addr, PeerSource::Manual);

		std::lock_guard<std::mutex> guard(peersMutex);
		connect(idx);
	}
}

size_t mtt::Peers::add(std::shared_ptr<TcpAsyncStream> stream, const BufferView& data)
{
	if (torrent->getState() == mttApi::Torrent::State::Inactive)
		return 0;

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, *peersListener, torrent->service.io);

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		KnownPeer p;
		p.address = stream->getAddress();
		p.source = PeerSource::Remote;
		knownPeers.push_back(p);

		peer.idx = (uint32_t)knownPeers.size() - 1;
		activeConnections.push_back(peer);
		addLogEvent(RemoteConnect, p.address, 0);

		remoteInfo.peers++;
	}

	return peer.comm->fromStream(stream, data);
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::disconnect(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto it = activeConnections.begin(); it != activeConnections.end(); it++)
	{
		if (it->comm.get() == p)
		{
			auto ptr = it->comm;
			auto& peer = knownPeers[it->idx];

			if (!p->state.finishedHandshake)
				peer.lastQuality = Peers::PeerQuality::Offline;
			else
			{
				p->close();
				peer.lastQuality = Peers::PeerQuality::Closed;
			}

			activeConnections.erase(it);
			addLogEvent(Remove, peer.address, (char)peer.lastQuality);

			return ptr;
		}
	}

	return nullptr;
}

std::vector<mtt::TrackerInfo> mtt::Peers::getSourcesInfo()
{
	torrent->loadFileInfo();

	std::vector<mtt::TrackerInfo> out;
	auto tr = trackers.getTrackers();
	out.reserve(tr.size() + 2);

	for (size_t i = 0; i < tr.size(); i++)
	{
		out.push_back(tr[i].second ? tr[i].second->info : mtt::TrackerInfo{});
		out.back().hostname = tr[i].first;
	}

	out.push_back(pexInfo);

	if(mtt::config::getExternal().dht.enabled)
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
		trackers.addTrackers(torrent->infoFile.announceList);

	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& peer : activeConnections)
	{
		if (peer.comm->state.finishedHandshake)
			peer.comm->info.pieces.resize(torrent->infoFile.info.pieces.size());
	}
}

uint32_t mtt::Peers::updateKnownPeers(const std::vector<Addr>& peers, PeerSource source)
{
	std::vector<uint32_t> accepted;
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		for (uint32_t i = 0; i < peers.size(); i++)
		{
			if (std::find(knownPeers.begin(), knownPeers.end(), peers[i]) == knownPeers.end())
				accepted.push_back(i);
		}

		if (accepted.empty())
			return 0;

		KnownPeer* addedPeersPtr;

		if (source == PeerSource::Pex || source == PeerSource::Dht)
		{
			knownPeers.insert(knownPeers.begin(), accepted.size(), KnownPeer());
			addedPeersPtr = &knownPeers[0];

			for (auto& conn : activeConnections)
			{
				conn.idx += (uint32_t)accepted.size();
			}
		}
		else
		{
			knownPeers.resize(knownPeers.size() + accepted.size());
			addedPeersPtr = &knownPeers[knownPeers.size() - accepted.size()];
		}

		for (uint32_t i = 0; i < accepted.size(); i++)
		{
			addedPeersPtr->address = peers[accepted[i]];
			PEERS_LOG("New peer " << addedPeersPtr->address.toString() << " source " << (int)source);
			addedPeersPtr->source = source;
			addedPeersPtr++;
		}
	}

	if(updateCallback)
		updateCallback(mtt::Status::Success, source);

	return (uint32_t)accepted.size();
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
	else
		it->lastQuality = PeerQuality::Unknown;
		
	return (uint32_t)std::distance(knownPeers.begin(), it);
}

void mtt::Peers::connect(uint32_t idx)
{
	auto& knownPeer = knownPeers[idx];
	if (knownPeer.lastQuality == PeerQuality::Unknown)
		knownPeer.lastQuality = PeerQuality::Connecting;

	PEERS_LOG("connect " << knownPeer.address.toString());

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, *peersListener, torrent->service.io);
	peer.comm->sendHandshake(knownPeer.address);
	peer.idx = idx;
	activeConnections.emplace_back(std::move(peer));

	addLogEvent(Connect, knownPeer.address, 0);
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::getActivePeer(const Addr& addr)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& connection : activeConnections)
	{
		auto& peer = knownPeers[connection.idx];
		if (peer.address == addr)
			return connection.comm;
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
	else
		return nullptr;
}

bool mtt::Peers::KnownPeer::operator==(const Addr& r)
{
	return address == r;
}

mtt::Peers::DhtSource::DhtSource(Peers& p, TorrentPtr t) : peers(p), torrent(t)
{
	info.hostname = "DHT";
	info.announceInterval = mtt::config::getInternal().dht.peersCheckInterval;
}

void mtt::Peers::DhtSource::start()
{
	std::lock_guard<std::mutex> guard(timerMtx);

	auto refreshFunc = [this]
	{
		std::lock_guard<std::mutex> guard(timerMtx);

		uint32_t currentTime = (uint32_t)time(0);
		uint32_t nextUpdate = info.announceInterval;

		if (info.nextAnnounce <= currentTime)
		{
			if (torrent->selectionFinished())
				info.nextAnnounce = currentTime + info.announceInterval;
			else
				findPeers();
		}
		else
			nextUpdate = info.nextAnnounce - currentTime;

		if (dhtRefreshTimer)
			dhtRefreshTimer->schedule(nextUpdate);
	};

	info.state = TrackerState::Connected;

	cfgCallbackId = mtt::config::registerOnChangeCallback(config::ValueType::Dht, refreshFunc);

	dhtRefreshTimer = ScheduledTimer::create(torrent->service.io, refreshFunc);

	dhtRefreshTimer->schedule(1);
}

void mtt::Peers::DhtSource::stop()
{
	{
		std::lock_guard<std::mutex> guard(timerMtx);

		if (dhtRefreshTimer)
			dhtRefreshTimer->disable();
		dhtRefreshTimer = nullptr;

		dht::Communication::get().stopFindingPeers(torrent->hash());
	}

	mtt::config::unregisterOnChangeCallback(cfgCallbackId);
	info.nextAnnounce = 0;
	info.state = TrackerState::Clear;
}

void mtt::Peers::DhtSource::findPeers()
{
	if (mtt::config::getExternal().dht.enabled && info.state != TrackerState::Announcing)
	{
		info.state = TrackerState::Announcing;
		dht::Communication::get().findPeers(torrent->hash(), this);
	}
}

uint32_t mtt::Peers::DhtSource::dhtFoundPeers(const uint8_t* hash, std::vector<Addr>& values)
{
	info.peers += peers.updateKnownPeers(values, PeerSource::Dht);
	return info.peers;
}

void mtt::Peers::DhtSource::dhtFindingPeersFinished(const uint8_t* hash, uint32_t count)
{
	uint32_t currentTime = (uint32_t)::time(0);
	info.lastAnnounce = currentTime;
	info.nextAnnounce = currentTime + info.announceInterval;
	info.state = TrackerState::Connected;
}

mtt::Peers::PeersListener::PeersListener(Peers* p) : peers(p)
{
}

void mtt::Peers::PeersListener::handshakeFinished(mtt::PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(mtx);

	if(peers)
	{
		std::lock_guard<std::mutex> guard(peers->peersMutex);
		if (auto peer = peers->getKnownPeer(p))
			peer->lastQuality = Peers::PeerQuality::Normal;
	}

	if (target)
		target->handshakeFinished(p);
}

void mtt::Peers::PeersListener::connectionClosed(mtt::PeerCommunication* p, int code)
{
	std::lock_guard<std::mutex> guard(mtx);

	if (peers)
	{
		auto ptr = peers->disconnect(p);

		if (target)
			target->connectionClosed(p, code);
	}
}

void mtt::Peers::PeersListener::messageReceived(mtt::PeerCommunication* p, mtt::PeerMessage& m)
{
	if (m.id == Port && m.port)
	{
		if (mtt::config::getExternal().dht.enabled)
		{
			Addr addr = p->getStream()->getAddress();
			addr.port = m.port;
			dht::Communication::get().pingNode(addr);
		}
	}

	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->messageReceived(p, m);
}

void mtt::Peers::PeersListener::extHandshakeFinished(mtt::PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->extHandshakeFinished(p);
}

void mtt::Peers::PeersListener::metadataPieceReceived(mtt::PeerCommunication* p, mtt::ext::UtMetadata::Message& m)
{
	std::lock_guard<std::mutex> guard(mtx);

	if (target)
		target->metadataPieceReceived(p, m);
}

void mtt::Peers::PeersListener::pexReceived(mtt::PeerCommunication* p, mtt::ext::PeerExchange::Message& msg)
{
	std::lock_guard<std::mutex> guard(mtx);

	if(peers)
		peers->pexInfo.peers += peers->updateKnownPeers(msg.addedPeers, PeerSource::Pex);

	if (target)
		target->pexReceived(p, msg);
}

void mtt::Peers::PeersListener::progressUpdated(mtt::PeerCommunication* p, uint32_t idx)
{
	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->progressUpdated(p, idx);
}

void mtt::Peers::PeersListener::setTarget(mtt::IPeerListener* t)
{
	std::lock_guard<std::mutex> guard(mtx);
	target = t;
}

void mtt::Peers::PeersListener::setParent(Peers* p)
{
	std::lock_guard<std::mutex> guard(mtx);
	peers = p;
}

#ifdef PEER_DIAGNOSTICS
void mtt::Peers::addLogEvent(LogEvent e, Addr& id, char info)
{
	std::lock_guard<std::mutex> guard(logmtx);

	logevents.push_back({ e, info,  id.port, id.toUint(), clock() });
}

extern std::string FormatLogTime(long);

void mtt::Peers::saveLogEvents()
{
	{
		std::lock_guard<std::mutex> guard(logmtx);

		if (logevents.empty())
			return;
	}

	std::ofstream file("logs\\" + torrent->name() + "\\peers.log");

	if (!file)
		return;

	{
		std::lock_guard<std::mutex> guard(logmtx);
		for (auto& e : logevents)
		{
			if (e.e == ConnectPeers)
				file << FormatLogTime(e.time) << ": Event:" << (int)e.e << " Want:" << e.id << " Remains:" << e.id2 << "\n";
			else
				file << FormatLogTime(e.time) << ": Event:" << (int)e.e << " Ip:" << Addr(e.id, e.id2).toString() << " Quality:" << (int)e.info << "\n";
		}
	}

	file << "\n\n\n";

	std::lock_guard<std::mutex> guard2(peersMutex);

	file << "Active:\n";
	for (auto& peer : activeConnections)
	{
		file << peer.comm->getAddressName() << " Idx:" << peer.idx << "\n";
	}

	file << "\n\n\n";
	file << "Known:\n";
	int i = 0;
	for (auto& peer : knownPeers)
	{
		file << i++ << " " << peer.address.toString() << " Q:" << (int)peer.lastQuality << " Conns:" << peer.connectionAttempts << "\n";
	}
}
#endif
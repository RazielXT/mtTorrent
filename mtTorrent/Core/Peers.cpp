#include "Peers.h"
#include "Torrent.h"
#include "PeerCommunication.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "Uploader.h"
#include "LogFile.h"


mtt::Peers::Peers(TorrentPtr t) : torrent(t), trackers(t), dht(*this, t), peersListener(*this)
{
	pexInfo.hostname = "PEX";
	pexInfo.state = TrackerState::Connected;
	trackers.addTrackers(t->infoFile.announceList);

	log.logName = "peers";
	log.clear();
}

void mtt::Peers::start(PeersUpdateCallback onPeersUpdated, IPeerListener* listener)
{
	updateCallback = onPeersUpdated;
	peersListener.setTarget(listener);
	trackers.start([this](Status s, AnnounceResponse* r, Tracker* t)
	{
		if (r)
		{
			LOG_APPEND(t->info.hostname << " returned " << r->peers.size());
			for (auto& a : r->peers)
			{
				LOG_APPEND(t->info.hostname << " " << a.toString());
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
}

void mtt::Peers::stop()
{
	trackers.stop();
	dht.stop();
	peersListener.setTarget(nullptr);
	updateCallback = nullptr;
}

void mtt::Peers::connectNext(uint32_t count)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	count = std::min(count, mtt::config::external.maxTorrentConnections - (uint32_t)activeConnections.size());

	auto currentTime = (uint32_t)std::time(0);
	uint32_t leastConnectionAttempts = 0;

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
		else if(p.lastQuality == PeerQuality::Offline)
			leastConnectionAttempts = p.connectionAttempts;
	}

	if (count > 0)
	{
		for (size_t i = 0; i < knownPeers.size(); i++)
		{
			if (count == 0)
				break;

			auto & p = knownPeers[i];
			if (p.lastQuality == PeerQuality::Offline && p.connectionAttempts <= leastConnectionAttempts && p.lastConnectionTime + 30 < currentTime)
			{
				connect((uint32_t)i);
				p.lastConnectionTime = currentTime;
				p.connectionAttempts++;
				count--;
			}
		}
	}
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::connect(Addr& addr)
{
	auto peer = getActivePeer(addr);

	if (!peer && addr.port != 0)
	{
		auto idx = updateKnownPeers(addr, PeerSource::Manual);

		std::lock_guard<std::mutex> guard(peersMutex);
		return connect(idx);
	}

	return peer;
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::getPeer(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& connection : activeConnections)
	{
		if (connection.comm.get() == p)
			return connection.comm;
	}

	return nullptr;
}

void mtt::Peers::add(std::shared_ptr<TcpAsyncStream> stream)
{
	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, peersListener);

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		KnownPeer p;
		p.address = Addr(stream->getEndpoint().address(), stream->getEndpoint().port());
		p.source = PeerSource::Remote;
		knownPeers.push_back(p);

		peer.idx = (uint32_t)knownPeers.size() - 1;
		activeConnections.push_back(peer);
	}

	peer.comm->setStream(stream);
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

			if (!p->isEstablished())
				peer.lastQuality = Peers::PeerQuality::Offline;
			else
			{
				p->stop();
				peer.lastQuality = Peers::PeerQuality::Unwanted;
			}

			activeConnections.erase(it);

			return ptr;
		}
	}

	return nullptr;
}

std::vector<mtt::TrackerInfo> mtt::Peers::getSourcesInfo()
{
	auto tr = trackers.getTrackers();
	std::vector<mtt::TrackerInfo> out;

	for (auto& t : tr)
	{
		if(t)
			out.push_back(t->info);
	}

	out.push_back(pexInfo);

	if(mtt::config::external.enableDht)
		out.push_back(dht.info);

	return out;
}

uint32_t mtt::Peers::getSourcesCount()
{
	return trackers.getTrackersCount() + (mtt::config::external.enableDht ? 2 : 1);
}

void mtt::Peers::refreshSource(const std::string& name)
{
	if (auto t = trackers.getTracker(name))
		t->announce();
	else if (name == "DHT")
		dht.findPeers();
}

uint32_t mtt::Peers::connectedCount()
{
	return (uint32_t)activeConnections.size();
}

uint32_t mtt::Peers::receivedCount()
{
	return (uint32_t)knownPeers.size();
}

uint32_t mtt::Peers::updateKnownPeers(std::vector<Addr>& peers, PeerSource source)
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
			addedPeersPtr->source = source;
			addedPeersPtr++;
		}
	}

	if(updateCallback)
		updateCallback(mtt::Status::Success, source);

	return (uint32_t)accepted.size();
}

uint32_t mtt::Peers::updateKnownPeers(Addr& addr, PeerSource source)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = std::find(knownPeers.begin(), knownPeers.end(), addr);

	if (it == knownPeers.end())
	{
		KnownPeer peer;
		peer.address = addr;
		peer.source = source;
		knownPeers.push_back(peer);

		return (uint32_t)knownPeers.size() - 1;
	}
	else
		it->lastQuality = PeerQuality::Unknown;
		
	return (uint32_t)std::distance(knownPeers.begin(), it);
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::connect(uint32_t idx)
{
	auto& knownPeer = knownPeers[idx];
	if (knownPeer.lastQuality == PeerQuality::Unknown)
		knownPeer.lastQuality = PeerQuality::Connecting;

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, peersListener, torrent->service.io);
	peer.comm->sendHandshake(knownPeer.address);
	peer.idx = idx;
	activeConnections.push_back(peer);

	return peer.comm;
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::getActivePeer(Addr& addr)
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
	return &knownPeers[getActivePeer(p)->idx];
}

mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(ActivePeer* p)
{
	return &knownPeers[p->idx];
}

bool mtt::Peers::KnownPeer::operator==(const Addr& r)
{
	return address == r;
}

mtt::Peers::DhtSource::DhtSource(Peers& p, TorrentPtr t) : peers(p), torrent(t)
{
	info.hostname = "DHT";
	info.state = TrackerState::Connected;
	info.announceInterval = mtt::config::internal_.dhtPeersCheckInterval;
}

void mtt::Peers::DhtSource::start()
{
	std::lock_guard<std::mutex> guard(timerMtx);

	dhtRefreshTimer = ScheduledTimer::create(torrent->service.io, [this]
		{
			std::lock_guard<std::mutex> guard(timerMtx);

			uint32_t currentTime = (uint32_t)time(0);
			uint32_t nextUpdate = info.announceInterval;

			if (info.nextAnnounce <= currentTime)
				findPeers();
			else
				nextUpdate = info.nextAnnounce - currentTime;
			
			dhtRefreshTimer->schedule(nextUpdate);
		}
	);

	dhtRefreshTimer->schedule(1);
}

void mtt::Peers::DhtSource::stop()
{
	std::lock_guard<std::mutex> guard(timerMtx);

	if (dhtRefreshTimer)
		dhtRefreshTimer->disable();
	dhtRefreshTimer = nullptr;
}

void mtt::Peers::DhtSource::findPeers()
{
	if (mtt::config::external.enableDht && info.state != TrackerState::Announcing)
	{
		info.state = TrackerState::Announcing;
		dht::Communication::get().findPeers(torrent->hash(), this);
	}
}

uint32_t mtt::Peers::DhtSource::dhtFoundPeers(uint8_t* hash, std::vector<Addr>& values)
{
	info.peers += peers.updateKnownPeers(values, PeerSource::Dht);
	return info.peers;
}

void mtt::Peers::DhtSource::dhtFindingPeersFinished(uint8_t* hash, uint32_t count)
{
	uint32_t currentTime = (uint32_t)std::time(0);
	info.lastAnnounce = currentTime;
	info.nextAnnounce = currentTime + info.announceInterval;
	info.state = TrackerState::Connected;
}

mtt::Peers::PeersListener::PeersListener(Peers& p) : peers(p)
{
}

void mtt::Peers::PeersListener::handshakeFinished(mtt::PeerCommunication* p)
{
	if (auto peer = peers.getKnownPeer(p))
		peer->lastQuality = Peers::PeerQuality::Normal;

	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->handshakeFinished(p);
}

void mtt::Peers::PeersListener::connectionClosed(mtt::PeerCommunication* p, int code)
{
	auto ptr = peers.disconnect(p);

	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->connectionClosed(p, code);
}

void mtt::Peers::PeersListener::messageReceived(mtt::PeerCommunication* p, mtt::PeerMessage& m)
{
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
	peers.pexInfo.peers += peers.updateKnownPeers(msg.addedPeers, PeerSource::Pex);

	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->pexReceived(p, msg);
}

void mtt::Peers::PeersListener::progressUpdated(mtt::PeerCommunication* p)
{
	//if (auto active = peers.getActivePeer(p))
	//	peers.knownPeers[active->idx].info.percentage = active->comm->info.pieces.getPercentage();

	std::lock_guard<std::mutex> guard(mtx);
	if (target)
		target->progressUpdated(p);
}

void mtt::Peers::PeersListener::setTarget(mtt::IPeerListener* t)
{
	target = t;
}

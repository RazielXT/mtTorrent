#include "Peers.h"
#include "Torrent.h"
#include "PeerCommunication.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "Uploader.h"

class DummyPeerListener : public mtt::IPeerListener
{
public:
	virtual void handshakeFinished(mtt::PeerCommunication*) override {}
	virtual void connectionClosed(mtt::PeerCommunication*, int code) override {}
	virtual void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override {}
	virtual void extHandshakeFinished(mtt::PeerCommunication*) override {}
	virtual void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override {}
	virtual void pexReceived(mtt::PeerCommunication*, mtt::ext::PeerExchange::Message&) override {}
	virtual void progressUpdated(mtt::PeerCommunication*) override {}
};

DummyPeerListener dummyListener;

mtt::Peers::Peers(TorrentPtr t) : torrent(t), trackers(t), analyzer(*this)
{
	dhtInfo.hostname = "DHT";
	dhtInfo.state = TrackerState::Connected;
	pexInfo.hostname = "PEX";
	pexInfo.state = TrackerState::Connected;
	trackers.addTrackers(t->infoFile.announceList);
}

void mtt::Peers::start(PeersUpdateCallback onPeersUpdated, IPeerListener* listener)
{
	updateCallback = onPeersUpdated;
	analyzer.peerListener = listener;
	trackers.start([this](Status s, AnnounceResponse* r, Tracker* t)
	{
		if (r)
		{
			updateKnownPeers(r->peers, PeerSource::Tracker);
		}

		updateCallback(s, t->info.hostname);
	}
	);

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

	analyzer.start();
}

void mtt::Peers::stop()
{
	trackers.stop();
	updateCallback = nullptr;
	analyzer.stop();
}

void mtt::Peers::connectNext(uint32_t count)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	count = std::min(count, mtt::config::external.maxTorrentConnections - (uint32_t)activeConnections.size());

	int lastConnections = 0;

	for (size_t i = 0; i < knownPeers.size(); i++)
	{
		if(count == 0)
			break;

		auto&p = knownPeers[i];
		if (p.lastQuality == PeerQuality::Unknown)// || p.connections < lastConnections)
		{
			connect((uint32_t)i);
			count--;
		}

		lastConnections = p.connections;
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
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, analyzer);

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		KnownPeer p;
		p.info.address = Addr(stream->getEndpoint().address(), stream->getEndpoint().port());
		p.info.source = PeerSource::Remote;
		knownPeers.push_back(p);

		peer.idx = (uint32_t)knownPeers.size() - 1;
		peer.timeConnected = (uint32_t)std::time(0);
		activeConnections.push_back(peer);
	}

	peer.comm->setStream(stream);
}

void mtt::Peers::disconnect(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto it = activeConnections.begin(); it != activeConnections.end(); it++)
	{
		if (it->comm.get() == p)
		{
			auto& peer = knownPeers[it->idx];

			if (!p->isEstablished())
				peer.lastQuality = Peers::PeerQuality::Offline;
			else
			{
				p->stop();
				peer.lastQuality = Peers::PeerQuality::Unwanted;
			}

			activeConnections.erase(it);

			break;
		}
	}
}

mtt::TrackerInfo mtt::Peers::getSourceInfo(const std::string& source)
{
	if (auto t = trackers.getTracker(source))
		return t->info;

	return dhtInfo;
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
		out.push_back(dhtInfo);

	return out;
}

uint32_t mtt::Peers::getSourcesCount()
{
	return trackers.getTrackersCount() + (mtt::config::external.enableDht ? 2 : 1);
}

uint32_t mtt::Peers::connectedCount()
{
	return (uint32_t)activeConnections.size();
}

uint32_t mtt::Peers::receivedCount()
{
	return (uint32_t)knownPeers.size();
}

std::vector<mtt::Peers::PeerInfo> mtt::Peers::getConnectedInfo()
{
	std::vector<mtt::Peers::PeerInfo> out;
	std::lock_guard<std::mutex> guard(peersMutex);
	out.resize(activeConnections.size());

	int i = 0;
	for (auto& p : activeConnections)
	{
		auto& info = out[i];
		auto& knownInfo = knownPeers[p.idx];
		info = knownInfo.info;
		i++;
	}

	return out;
}

uint32_t mtt::Peers::updateKnownPeers(std::vector<Addr>& peers, PeerSource source)
{
	std::vector<uint32_t> accepted;
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
		addedPeersPtr->info.address = peers[accepted[i]];
		addedPeersPtr->info.source = source;
		addedPeersPtr++;
	}

	return (uint32_t)accepted.size();
}

uint32_t mtt::Peers::updateKnownPeers(Addr& addr, PeerSource source)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto it = std::find(knownPeers.begin(), knownPeers.end(), addr);

	if (it == knownPeers.end())
	{
		KnownPeer peer;
		peer.info.address = addr;
		peer.info.source = source;
		knownPeers.push_back(peer);

		return (uint32_t)knownPeers.size() - 1;
	}

	return (uint32_t)std::distance(knownPeers.begin(), it);
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::connect(uint32_t idx)
{
	auto& knownPeer = knownPeers[idx];
	if (knownPeer.lastQuality == PeerQuality::Unknown)
		knownPeer.lastQuality = PeerQuality::Potential;
	knownPeer.connections++;

	ActivePeer peer;
	peer.comm = std::make_shared<PeerCommunication>(torrent->infoFile.info, analyzer, torrent->service.io);
	peer.comm->sendHandshake(knownPeer.info.address);
	peer.idx = idx;
	peer.timeConnected = (uint32_t)std::time(0);
	activeConnections.push_back(peer);

	return peer.comm;
}

std::shared_ptr<mtt::PeerCommunication> mtt::Peers::getActivePeer(Addr& addr)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	for (auto& connection : activeConnections)
	{
		auto& peer = knownPeers[connection.idx];
		if (peer.info.address == addr)
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

bool mtt::Peers::KnownPeer::operator==(const Addr& r)
{
	return info.address == r;
}

mtt::PeersAnalyzer::PeersAnalyzer(Peers& p) : peers(p)
{
	peerListener = &dummyListener;
}

void mtt::PeersAnalyzer::handshakeFinished(PeerCommunication* p)
{
	{
		std::lock_guard<std::mutex> guard(peers.peersMutex);

		auto peer = peers.getKnownPeer(p);
		peer->lastQuality = Peers::PeerQuality::Normal;
	}

	if(!peers.torrent->files.progress.empty())
		p->sendBitfield(peers.torrent->files.progress.toBitfield());

	peerListener->handshakeFinished(p);
}

void mtt::PeersAnalyzer::connectionClosed(PeerCommunication* p, int code)
{
	peers.disconnect(p);

	peerListener->connectionClosed(p, code);
}

void mtt::PeersAnalyzer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	if (msg.id == Piece)
	{
		std::lock_guard<std::mutex> guard(peers.peersMutex);

		auto peer = peers.getKnownPeer(p);
		peer->downloaded += msg.piece.info.length;
	}
	else if (msg.id == Unchoke)
	{
		peers.torrent->uploader->wantsUnchoke(p);
	}
	else if (msg.id == Request)
	{
		if (peers.torrent->uploader->pieceRequest(p, msg.request))
		{
			std::lock_guard<std::mutex> guard(peers.peersMutex);

			auto peer = peers.getKnownPeer(p);
			peer->uploaded += msg.request.length;
			uploaded += msg.request.length;
		}
	}

	peerListener->messageReceived(p, msg);
}

void mtt::PeersAnalyzer::extHandshakeFinished(PeerCommunication* p)
{
	peerListener->extHandshakeFinished(p);
}

void mtt::PeersAnalyzer::metadataPieceReceived(PeerCommunication* p, ext::UtMetadata::Message& msg)
{
	peerListener->metadataPieceReceived(p, msg);
}

void mtt::PeersAnalyzer::pexReceived(PeerCommunication* p, ext::PeerExchange::Message& msg)
{
	peers.pexInfo.peers += peers.updateKnownPeers(msg.addedPeers, PeerSource::Pex);
	peerListener->pexReceived(p, msg);
}

void mtt::PeersAnalyzer::progressUpdated(PeerCommunication* p)
{
	{
		std::lock_guard<std::mutex> guard(peers.peersMutex);

		if (auto active = peers.getActivePeer(p))
			peers.knownPeers[active->idx].info.percentage = active->comm->info.pieces.getPercentage();
	}

	peerListener->progressUpdated(p);
}

uint32_t mtt::PeersAnalyzer::getDownloadSpeed(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peers.peersMutex);

	for (auto peer : peers.activeConnections)
	{
		if (peer.comm.get() == p)
		{
			return peers.knownPeers[peer.idx].info.downloadSpeed;
		}
	}

	return 0;
}

uint32_t mtt::PeersAnalyzer::getDownloadSpeed()
{
	std::lock_guard<std::mutex> guard(peers.peersMutex);
	uint32_t speed = 0;
	for (auto peer : peers.activeConnections)
	{
		speed += peers.knownPeers[peer.idx].info.downloadSpeed;
	}

	return speed;
}

uint32_t mtt::PeersAnalyzer::getUploadSpeed()
{
	std::lock_guard<std::mutex> guard(peers.peersMutex);
	uint32_t speed = 0;
	for (auto peer : peers.activeConnections)
	{
		speed += peers.knownPeers[peer.idx].info.uploadSpeed;
	}

	return speed;
}

size_t mtt::PeersAnalyzer::getUploadSum()
{
	return uploaded;
}

void mtt::PeersAnalyzer::start()
{
	std::lock_guard<std::mutex> guard(measureMutex);
	speedMeasureTimer = ScheduledTimer::create(peers.torrent->service.io, [this]
	{
		updateMeasures();
		evalCurrentPeers();
		checkForDhtPeers();

		std::lock_guard<std::mutex> guard(measureMutex);
		if(speedMeasureTimer)
			speedMeasureTimer->schedule(1);
	}
	);

	speedMeasureTimer->schedule(1);
}

void mtt::PeersAnalyzer::stop()
{
	std::lock_guard<std::mutex> guard(measureMutex);
	if(speedMeasureTimer)
		speedMeasureTimer->disable();
	speedMeasureTimer = nullptr;
	peerListener = &dummyListener;
}

void mtt::PeersAnalyzer::updateMeasures()
{
	auto& freshPieces = peers.torrent->files.freshPieces;
	std::vector<std::pair<PeerCommunication*, std::pair<size_t,size_t>>> currentMeasure;

	{
		std::lock_guard<std::mutex> guard(peers.peersMutex);
		for (auto peer : peers.activeConnections)
		{
			auto& peerInfo = peers.knownPeers[peer.idx];
			currentMeasure.push_back({ peer.comm.get(), {peerInfo.downloaded, peerInfo.uploaded} });
			peerInfo.info.downloadSpeed = 0;
			peerInfo.info.uploadSpeed = 0;

			for (auto last : lastSpeedMeasure)
			{
				if (last.first == peer.comm.get())
				{
					peerInfo.info.downloadSpeed = (uint32_t)(peerInfo.downloaded - last.second.first);
					peerInfo.info.uploadSpeed = (uint32_t)(peerInfo.uploaded - last.second.second);
					break;
				}
			}

			for (auto& piece : freshPieces)
				if(peer.comm->isEstablished())
					peer.comm->sendHave(piece);
		}
	}

	freshPieces.clear();
	lastSpeedMeasure = currentMeasure;
}

void mtt::PeersAnalyzer::evalCurrentPeers()
{
	const uint32_t peersEvalInterval = 5;

	secondsFromLastPeerEval++;
	if (secondsFromLastPeerEval < peersEvalInterval)
		return;

	secondsFromLastPeerEval = 0;

	PeerCommunication* slowestPeer = nullptr;
	{
		std::lock_guard<std::mutex> guard(peers.peersMutex);

		if ((uint32_t)peers.activeConnections.size() < mtt::config::external.maxTorrentConnections)
			return;

		const uint32_t minPeersTimeChance = 10;
		uint32_t currentTime = (uint32_t)std::time(0);
		auto minTimeToEval = currentTime - minPeersTimeChance;

		uint32_t slowestSpeed = -1;

		for (auto peer : peers.activeConnections)
		{
			if(peer.timeConnected > minTimeToEval)
				continue;

			auto speed = peers.knownPeers[peer.idx].info.downloadSpeed;
			if (speed < slowestSpeed)
			{
				slowestSpeed = speed;
				slowestPeer = peer.comm.get();
			}
		}
	}

	if (slowestPeer != nullptr)
		peers.disconnect(slowestPeer);
}

void mtt::PeersAnalyzer::checkForDhtPeers()
{
	const uint32_t dhtCheckInterval = 60;

	secondsFromLastDhtCheck++;
	if (secondsFromLastDhtCheck < dhtCheckInterval)
		return;

	if (peers.torrent->infoFile.info.name.empty())
		return;

	secondsFromLastDhtCheck = 0;
	peers.dhtInfo.state = TrackerState::Announcing;
	uint32_t currentTime = (uint32_t)std::time(0);
	peers.dhtInfo.lastAnnounce = currentTime;
	peers.dhtInfo.nextAnnounce = currentTime + dhtCheckInterval;
	peers.dhtInfo.announceInterval = dhtCheckInterval;

	dht::Communication::get().findPeers(peers.torrent->infoFile.info.hash, this);
}

uint32_t mtt::PeersAnalyzer::dhtFoundPeers(uint8_t* hash, std::vector<Addr>& values)
{
	peers.dhtInfo.peers += peers.updateKnownPeers(values, PeerSource::Dht);
	return peers.dhtInfo.peers;
}

void mtt::PeersAnalyzer::dhtFindingPeersFinished(uint8_t* hash, uint32_t count)
{
	peers.dhtInfo.state = TrackerState::Connected;
}

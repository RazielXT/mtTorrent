#include "FileTransfer.h"
#include "Torrent.h"
#include "Files.h"
#include "PeerCommunication.h"
#include "Peers.h"
#include "Configuration.h"
#include "utils/ScheduledTimer.h"
#include "utils/FastIpToCountry.h"
#include <fstream>

FastIpToCountry ipToCountry;
bool ipToCountryLoaded = false;

mtt::FileTransfer::FileTransfer(TorrentPtr t) : downloader(t), uploader(t), torrent(t)
{
	log.init("download");

	if (!ipToCountryLoaded)
	{
		ipToCountryLoaded = true;
		ipToCountry.fromFile(mtt::config::getInternal().programFolderPath);
	}
}

void mtt::FileTransfer::start()
{
	piecesAvailability.resize(torrent->infoFile.info.pieces.size());
	downloader.reset();

	torrent->peers->start([this](Status s, mtt::PeerSource)
		{
			if (s == Status::Success)
			{
				evaluateCurrentPeers();
			}
		}
	, this);

	refreshTimer = ScheduledTimer::create(torrent->service.io, [this]
		{
			evalCurrentPeers();
			updateMeasures();

			refreshTimer->schedule(1);
		}
	);

	refreshTimer->schedule(1);
}

void mtt::FileTransfer::stop()
{
	saveLogEvents();

	torrent->peers->stop();
	downloader.reset();
	torrent->files.storage.flush();

	if(refreshTimer)
		refreshTimer->disable();

	std::lock_guard<std::mutex> guard(peersMutex);
	activePeers.clear();
}

void mtt::FileTransfer::reevaluate()
{
	std::lock_guard<std::mutex> guard(peersMutex);
	for(auto& p : activePeers)
		downloader.evaluateNextRequests(&p);
}

void mtt::FileTransfer::handshakeFinished(PeerCommunication* p)
{
	LOG_APPEND("handshake " << p->getAddressName());
	if (!torrent->files.progress.empty())
		p->sendBitfield(torrent->files.progress.toBitfield());

	addPeer(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	LOG_APPEND("closed " << p->getAddressName());
	removePeer(p);
}

void mtt::FileTransfer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	LOG_APPEND("msg " << msg.id << " " << p->getAddressName());

	if (msg.id == Piece)
	{
		LOG_APPEND("piece " << msg.piece.info.index << " " << msg.piece.info.begin << " " << p->getAddressName());

		auto status = downloader.pieceBlockReceived(msg.piece);

		std::lock_guard<std::mutex> guard(peersMutex);
		downloader.removeBlockRequests(activePeers, msg.piece, status, p);

		if (auto peer = getActivePeer(p))
		{
			peer->downloaded += msg.piece.info.length;
			peer->lastActivityTime = (uint32_t)time(0);
		}
	}
	else if (msg.id == Unchoke)
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		if (auto peer = getActivePeer(p))
		{
			downloader.evaluateNextRequests(peer);
			peer->lastActivityTime = (uint32_t)time(0);
		}
	}
	else if (msg.id == Interested)
	{
		uploader.isInterested(p);
	}
	else if (msg.id == Request)
	{
		if (uploader.pieceRequest(p, msg.request))
		{
			std::lock_guard<std::mutex> guard(peersMutex);
			if (auto peer = getActivePeer(p))
				peer->uploaded += msg.request.length;
		}
	}
}

void mtt::FileTransfer::extHandshakeFinished(PeerCommunication*)
{
}

void mtt::FileTransfer::metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&)
{
}

void mtt::FileTransfer::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::FileTransfer::progressUpdated(PeerCommunication* p, uint32_t idx)
{
	if (idx != -1)
		piecesAvailability[idx]++;
	else
	{
		auto& peerPieces = p->info.pieces.pieces;

		for(size_t i = 0; i < piecesAvailability.size(); i++)
		{
			piecesAvailability[i] += peerPieces[i] ? 1 : 0;
		}
	}

	evaluateNextRequests(p);
}

size_t mtt::FileTransfer::getUploadSum()
{
	return uploader.uploaded;
}

size_t mtt::FileTransfer::getDownloadSpeed()
{
	size_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.downloadSpeed;

	return sum;
}

size_t mtt::FileTransfer::getUploadSpeed()
{
	size_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.uploadSpeed;

	return sum;
}

std::vector<mtt::FileTransfer::ActivePeerInfo> mtt::FileTransfer::getPeersInfo()
{
	std::vector<mtt::FileTransfer::ActivePeerInfo> out;

	std::lock_guard<std::mutex> guard(peersMutex);
	out.resize(activePeers.size());

	uint32_t i = 0;
	for (auto& peer : activePeers)
	{
		out[i].address = peer.comm->getAddress();
		out[i].percentage = peer.comm->info.pieces.getPercentage();
		out[i].downloadSpeed = peer.downloadSpeed;
		out[i].uploadSpeed = peer.uploadSpeed;
		out[i].client = peer.comm->ext.state.client;
		out[i].country = out[i].address.ipv6 ? "" : ipToCountry.GetCountry(_byteswap_ulong(*reinterpret_cast<uint32_t*>(out[i].address.addrBytes)));
		i++;
	}

	return out;
}

std::vector<uint32_t> mtt::FileTransfer::getCurrentRequests()
{
	return downloader.getCurrentRequests();
}

uint32_t mtt::FileTransfer::getCurrentRequestsCount()
{
	return downloader.getCurrentRequestsCount();
}

mtt::ActivePeer* mtt::FileTransfer::getActivePeer(PeerCommunication* p)
{
	for (auto& peer : activePeers)
		if (peer.comm == p)
			return &peer;

	return nullptr;
}

void mtt::FileTransfer::addPeer(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	bool found = false;
	for (auto& peer : activePeers)
	{
		if (peer.comm == p)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		activePeers.push_back({ p,{} });
		activePeers.back().connectionTime = activePeers.back().lastActivityTime = (uint32_t)time(0);
		downloader.evaluateNextRequests(&activePeers.back());
	}
}

void mtt::FileTransfer::evaluateNextRequests(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	if (auto peer = getActivePeer(p))
		downloader.evaluateNextRequests(peer);
}

void mtt::FileTransfer::removePeer(PeerCommunication * p)
{
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		for (auto it = activePeers.begin(); it != activePeers.end(); it++)
		{
			if (it->comm == p)
			{
				activePeers.erase(it);
				break;
			}
		}
	}

	evaluateCurrentPeers();
}

void mtt::FileTransfer::removePeers(std::vector<uint32_t> sortedIdx)
{
	for (auto it = sortedIdx.rbegin(); it != sortedIdx.rend(); it++)
	{
		activePeers.erase(activePeers.begin() + *it);
	}
}

void mtt::FileTransfer::evaluateCurrentPeers()
{
	if (activePeers.size() < mtt::config::getExternal().connection.maxTorrentConnections && !torrent->selectionFinished())
		torrent->peers->connectNext(10);
}

void mtt::FileTransfer::evalCurrentPeers()
{
	const uint32_t peersEvalInterval = 5;

	if (peersEvalCounter-- > 0)
		return;

	peersEvalCounter = peersEvalInterval;

	std::vector<uint32_t> removedPeers;
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		uint32_t currentTime = (uint32_t)::time(0);

#ifdef PEER_DIAGNOSTICS
		std::lock_guard<std::mutex> guardLog(logmtx);
		logEvals.push_back({ clock(), (uint32_t)activePeers.size() });

		size_t logStartIdx = logEvalPeers.size();

		if (activePeers.size())
		{
			logEvalPeers.resize(logEvalPeers.size() + activePeers.size());

			size_t idx = logStartIdx;
			for (auto peer : activePeers)
			{
				logEvalPeers[idx].addr = peer.comm->getAddress();
				logEvalPeers[idx].dl = peer.downloadSpeed;
				logEvalPeers[idx].up = peer.uploadSpeed;
				logEvalPeers[idx].activityTime = currentTime - peer.lastActivityTime;
				idx++;
			}
		}
#endif

		if ((uint32_t)activePeers.size() < mtt::config::getExternal().connection.maxTorrentConnections)
		{
			return;
		}

		const uint32_t minPeersTimeChance = 10;
		
		auto minTimeToEval = currentTime - minPeersTimeChance;

		const uint32_t minPeersPiecesTimeChance = 20;
		auto minTimeToReceive = currentTime - minPeersPiecesTimeChance;

		uint32_t slowestSpeed = -1;
		uint32_t slowestPeer = -1;

		uint32_t maxUploads = 5;
		std::vector<uint32_t> currentUploads;

		uint32_t idx = 0;
		for (auto peer : activePeers)
		{
			if (peer.connectionTime > minTimeToEval)
			{
#ifdef PEER_DIAGNOSTICS
				logEvalPeers[logStartIdx + idx].action = LogEvalPeer::TooSoon;
#endif
				continue;
			}

			if (peer.comm->state.peerInterested)
			{
				currentUploads.push_back(idx);
			}

			if (peer.lastActivityTime < minTimeToReceive)
			{
#ifdef PEER_DIAGNOSTICS
				logEvalPeers[logStartIdx + idx].action = LogEvalPeer::NotResponding;
#endif
				removedPeers.push_back(idx);
				continue;
			}

			if (peer.downloadSpeed < slowestSpeed)
			{
				slowestSpeed = peer.downloadSpeed;
				slowestPeer = idx;
			}

			idx++;
		}

		if (removedPeers.empty() && slowestPeer != -1)
		{
#ifdef PEER_DIAGNOSTICS
			logEvalPeers[logStartIdx + idx].action = LogEvalPeer::TooSlow;
#endif
			removedPeers.push_back(slowestPeer);
		}

		if (!currentUploads.empty())
		{
			std::sort(currentUploads.begin(), currentUploads.end(), [&](const auto & l, const auto & r) { return activePeers[l].uploadSpeed > activePeers[r].uploadSpeed; });
			currentUploads.resize(std::min((uint32_t)currentUploads.size(), maxUploads));

			for (auto idx : currentUploads)
			{
				auto it = std::find(removedPeers.begin(), removedPeers.end(), idx);
				if (it != removedPeers.end())
					removedPeers.erase(it);

#ifdef PEER_DIAGNOSTICS
				logEvalPeers[logStartIdx + idx].action = LogEvalPeer::Upload;
#endif
			}
		}

		removePeers(removedPeers);
	}

	if (!removedPeers.empty())
		evaluateCurrentPeers();
}

void mtt::FileTransfer::updateMeasures()
{
	auto& freshPieces = torrent->files.freshPieces;
	std::vector<std::pair<PeerCommunication*, std::pair<size_t, size_t>>> currentMeasure;

	{
		std::lock_guard<std::mutex> guard(peersMutex);
		for (auto& peer : activePeers)
		{
			currentMeasure.push_back({ peer.comm, {peer.downloaded, peer.uploaded} });
			peer.downloadSpeed = 0;
			peer.uploadSpeed = 0;

			for (auto last : lastSpeedMeasure)
			{
				if (last.first == peer.comm)
				{
					if (peer.downloaded > last.second.first)
						peer.downloadSpeed = (uint32_t)(peer.downloaded - last.second.first);
					if (peer.uploaded > last.second.second)
						peer.uploadSpeed = (uint32_t)(peer.uploaded - last.second.second);

					break;
				}
			}

			for (auto& piece : freshPieces)
				if (peer.comm->isEstablished())
					peer.comm->sendHave(piece);
		}
	}
	freshPieces.clear();
	lastSpeedMeasure = currentMeasure;

	if (!torrent->selectionFinished())
		downloader.sortPriorityByAvailability(piecesAvailability);
}

#ifdef PEER_DIAGNOSTICS
extern std::string FormatLogTime(long);

void mtt::FileTransfer::saveLogEvents()
{
	std::lock_guard<std::mutex> guard(logmtx);

	if (logEvals.empty())
		return;

	std::ofstream file("logs\\" + torrent->name() + "\\downloader.log");

	if (!file)
		return;

	size_t logIndex = 0;

	for (auto& eval : logEvals)
	{
		file << FormatLogTime(eval.time) << ": Count:" << eval.count << "\n";

		for (size_t i = logIndex; i < logIndex + eval.count; i++)
		{
			auto& peerLog = logEvalPeers[i];

			file << peerLog.addr.toString() << " Action:" << (int)peerLog.action << " dl:" << peerLog.dl/1024.f << " up:" << peerLog.up/ 1024.f << " dead:" << peerLog.activityTime << "\n";
		}

		logIndex += eval.count;
	}

	file << "\n\n";
}
#endif
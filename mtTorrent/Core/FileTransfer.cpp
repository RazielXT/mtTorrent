#include "FileTransfer.h"
#include "Torrent.h"
#include "Files.h"
#include "PeerCommunication.h"
#include "Peers.h"
#include "Configuration.h"
#include "utils/ScheduledTimer.h"

mtt::FileTransfer::FileTransfer(TorrentPtr t) : downloader(t), uploader(t), torrent(t)
{

}

void mtt::FileTransfer::start()
{
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
	torrent->peers->stop();
	downloader.reset();
	torrent->files.storage.flush();

	std::lock_guard<std::mutex> guard(peersMutex);
	activePeers.clear();
}

void mtt::FileTransfer::handshakeFinished(PeerCommunication* p)
{
	if (!torrent->files.progress.empty())
		p->sendBitfield(torrent->files.progress.toBitfield());

	addPeer(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	removePeer(p);
}

void mtt::FileTransfer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	if (msg.id == Piece)
	{
		auto status = downloader.pieceBlockReceived(msg.piece);

		std::lock_guard<std::mutex> guard(peersMutex);
		downloader.removeBlockRequests(activePeers, msg.piece, status, p);

		if (auto peer = getActivePeer(p))
			peer->downloaded += msg.piece.info.length;
	}
	else if (msg.id == Unchoke)
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		if(auto peer = getActivePeer(p))
			downloader.evaluateNextRequests(peer);
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

void mtt::FileTransfer::progressUpdated(PeerCommunication* p)
{
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
		i++;
	}

	return out;
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

void mtt::FileTransfer::evaluateCurrentPeers()
{
	if (activePeers.size() < mtt::config::external.maxTorrentConnections)
		torrent->peers->connectNext(10);
}

void mtt::FileTransfer::evalCurrentPeers()
{
	const uint32_t peersEvalInterval = 5;

	if (peersEvalCounter-- > 0)
		return;

	peersEvalCounter = peersEvalInterval;

	std::vector<PeerCommunication*> removePeers;
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		if ((uint32_t)activePeers.size() < mtt::config::external.maxTorrentConnections)
			return;

		const uint32_t minPeersTimeChance = 10;
		uint32_t currentTime = (uint32_t)std::time(0);
		auto minTimeToEval = currentTime - minPeersTimeChance;

		const uint32_t minPeersPiecesTimeChance = 20;
		auto minTimeToReceive = currentTime - minPeersPiecesTimeChance;

		uint32_t slowestSpeed = -1;
		PeerCommunication* slowestPeer;

		uint32_t maxUploads = 5;
		std::vector<ActivePeer*> currentUploads;

		for (auto peer : activePeers)
		{
			if (peer.connectionTime > minTimeToEval)
				continue;

			if (peer.lastActivityTime < minTimeToReceive)
			{
				removePeers.push_back(peer.comm);
				continue;
			}

			if (peer.comm->state.peerInterested)
			{
				currentUploads.push_back(&peer);
			}

			if (peer.downloadSpeed < slowestSpeed)
			{
				slowestSpeed = peer.downloadSpeed;
				slowestPeer = peer.comm;
			}
		}

		if (removePeers.empty())
			removePeers.push_back(slowestPeer);

		if (!currentUploads.empty())
		{
			std::sort(currentUploads.begin(), currentUploads.end(), [](const auto & l, const auto & r) { return l->uploadSpeed > r->uploadSpeed; });
			currentUploads.resize(std::min((uint32_t)currentUploads.size(), maxUploads));

			for (auto p : currentUploads)
			{
				auto it = std::find(removePeers.begin(), removePeers.end(), p->comm);
				if (it != removePeers.end())
					removePeers.erase(it);
			}
		}
	}

	for (auto p : removePeers)
	{
		removePeer(p);
	}
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
}

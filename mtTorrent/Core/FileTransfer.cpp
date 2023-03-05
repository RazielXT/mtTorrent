#include "FileTransfer.h"
#include "Torrent.h"
#include "Files.h"
#include "PeerCommunication.h"
#include "MetadataExtension.h"
#include "Peers.h"
#include "Configuration.h"
#include "AlertsManager.h"

#define TRANSFER_LOG(x) WRITE_LOG(x)

mtt::FileTransfer::FileTransfer(Torrent& t) : downloader(t), torrent(t)
{
	uploader = std::make_shared<Uploader>(torrent);

	CREATE_NAMED_LOG(Transfer, torrent.name());
}

void mtt::FileTransfer::start()
{
	piecesAvailability.resize(torrent.infoFile.info.pieces.size());
	refreshSelection();

	if (refreshTimer)
		return;

	torrent.peers->start(this);

	refreshTimer = ScheduledTimer::create(torrent.service.io, [this]
		{
			evalCurrentPeers();
			updateMeasures();
			uploader->refreshRequest();

			return ScheduledTimer::DurationSeconds(1);
		}
	);

	refreshTimer->schedule(ScheduledTimer::DurationSeconds(1));
	evaluateMorePeers();
}

void mtt::FileTransfer::stop()
{
	torrent.peers->stop();

	if (refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	downloader.stop();
	uploader->stop();
}

void mtt::FileTransfer::refreshSelection()
{
	if (!piecesAvailability.empty())
		downloader.refreshSelection(torrent.files.selection, piecesAvailability);

	evaluateMorePeers();
}

void mtt::FileTransfer::handshakeFinished(PeerCommunication* p)
{
	if (!refreshTimer)
		return;

	TRANSFER_LOG("handshake " << p->getStream()->getAddressName());
	if (!torrent.files.progress.empty())
		p->sendBitfield(torrent.files.progress.toBitfieldData());

	addPeer(p);
	downloader.handshakeFinished(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	TRANSFER_LOG("closed " << p->getStream()->getAddressName());
	removePeer(p);
	downloader.connectionClosed(p);
	uploader->cancelRequests(p);
}

void mtt::FileTransfer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	if (!refreshTimer)
		return;

	TRANSFER_LOG("msg " << msg.id << " " << p->getStream()->getAddressName());

	if (msg.id == PeerMessage::Interested)
	{
		uploader->isInterested(p);
	}
	else if (msg.id == PeerMessage::Request)
	{
		uploader->pieceRequest(p, msg.request);
	}
	else if (msg.id == PeerMessage::Have)
	{
		if (msg.havePieceIndex < piecesAvailability.size())
			piecesAvailability[msg.havePieceIndex]++;
	}
	else if (msg.id == PeerMessage::Bitfield)
	{
		const auto& peerPieces = p->info.pieces.pieces;

		for (size_t i = 0; i < piecesAvailability.size(); i++)
			piecesAvailability[i] += peerPieces[i] ? 1 : 0;
	}

	downloader.messageReceived(p, msg);
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		activePeers[p].lastActivityTime = mtt::CurrentTimestamp();
	}
}

void mtt::FileTransfer::extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&)
{
}

void mtt::FileTransfer::extendedMessageReceived(PeerCommunication* p, ext::Type type, const BufferView& data)
{
	if (type == ext::Type::UtMetadata)
	{
		ext::UtMetadata::Message msg;

		if (ext::UtMetadata::Load(data, msg))
		{
			if (msg.type == ext::UtMetadata::Type::Request)
			{
				const uint32_t offset = msg.piece * ext::UtMetadata::PieceSize;
				auto& data = torrent.infoFile.info.data;

				if (offset < data.size())
				{
					size_t pieceSize = std::min<size_t>(ext::UtMetadata::PieceSize, torrent.infoFile.info.data.size() - offset);
					p->ext.utm.sendPieceResponse(msg.piece, { data.data() + offset, pieceSize }, data.size());
				}
			}
		}
	}
}

uint32_t mtt::FileTransfer::getDownloadSpeed() const
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.second.downloadSpeed;

	return sum;
}

uint32_t mtt::FileTransfer::getUploadSpeed() const
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.second.uploadSpeed;

	return sum;
}

std::map<mtt::PeerCommunication*, std::pair<uint32_t, uint32_t>> mtt::FileTransfer::getPeersSpeeds() const
{
	std::lock_guard<std::mutex> guard(peersMutex);

	std::map<mtt::PeerCommunication*, std::pair<uint32_t, uint32_t>> out;
	for (auto&[comm, info] : activePeers)
	{
		out[comm] = { info.downloadSpeed, info.uploadSpeed };
	}
	return out;
}

void mtt::FileTransfer::addPeer(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	auto& info = activePeers[p];
	info.connectionTime = info.lastActivityTime = mtt::CurrentTimestamp();
}

void mtt::FileTransfer::removePeer(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);

	if (auto it = activePeers.find(p); it != activePeers.end())
		activePeers.erase(it);
}

void mtt::FileTransfer::evaluateMorePeers()
{
	TRANSFER_LOG("evaluateMorePeers");
	if (!torrent.selectionFinished() && !torrent.files.checking)
		torrent.peers->startConnecting();
	else
		torrent.peers->stopConnecting();
}

void mtt::FileTransfer::evalCurrentPeers()
{
	TRANSFER_LOG("evalCurrentPeers");
	const uint32_t peersEvalInterval = 5;

	if (peersEvalCounter-- > 0)
		return;

	peersEvalCounter = peersEvalInterval;
	{
		const auto currentTime = mtt::CurrentTimestamp();

		const uint32_t minPeersTimeChance = 10;
		auto minTimeToEval = currentTime - minPeersTimeChance;

		const uint32_t minPeersPiecesTimeChance = 20;
		auto minTimeToReceive = currentTime - minPeersPiecesTimeChance;

		std::lock_guard<std::mutex> guard(peersMutex);
		TRANSFER_LOG("evalCurrentPeers" << activePeers.size());

		if (activePeers.size() > mtt::config::getExternal().connection.maxTorrentConnections)
		{
			struct PeerEval
			{
				PeerCommunication* comm = nullptr;
				const ActivePeer* info = nullptr;
			};
			PeerEval slowestPeer;
			std::vector<PeerEval> idleUploads;
			std::vector<PeerEval> idlePeers;

			for (const auto& [comm,peer] : activePeers)
			{
				TRANSFER_LOG(comm->getStream()->getAddress() << peer.downloadSpeed << peer.uploadSpeed << currentTime - peer.lastActivityTime);

				if (peer.connectionTime > minTimeToEval)
				{
					continue;
				}

				if (peer.lastActivityTime < minTimeToReceive)
				{
					if (comm->state.peerInterested)
						idleUploads.push_back({ comm, &peer });
					else
						idlePeers.push_back({ comm, &peer });

					continue;
				}

				if (!slowestPeer.info ||
					peer.downloadSpeed < slowestPeer.info->downloadSpeed || 
					(peer.downloadSpeed == slowestPeer.info->downloadSpeed && peer.downloaded < slowestPeer.info->downloaded))
				{
					slowestPeer = { comm, &peer };
				}
			}

			const uint32_t maxProtectedUploads = 5;
			if (idleUploads.size() > maxProtectedUploads)
			{
				std::sort(idleUploads.begin(), idleUploads.end(), [](const auto& l, const auto& r)
					{ return r.info->uploadSpeed > l.info->uploadSpeed || (r.info->uploadSpeed == l.info->uploadSpeed && r.info->uploaded > l.info->uploaded); });

				idleUploads.resize(idleUploads.size() - maxProtectedUploads);
				for (const auto& peer : idleUploads)
				{
					TRANSFER_LOG("eval upload" << peer.comm->getStream()->getAddressName());
					idlePeers.push_back(peer);
				}
			}

			if (idlePeers.size() > 5)
			{
				std::sort(idlePeers.begin(), idlePeers.end(),	[](const auto& l, const auto& r)
					{ return l.info->downloaded < r.info->downloaded; });

				idlePeers.resize(5);
			}
			else if (idlePeers.empty() && slowestPeer.comm)
			{
				TRANSFER_LOG("eval slowest" << slowestPeer.comm->getStream()->getAddressName());
				idlePeers.push_back(slowestPeer);
			}

			for (const auto& peer : idlePeers)
			{
				TRANSFER_LOG("eval removing" << peer.comm->getStream()->getAddressName());
				torrent.peers->disconnect(peer.comm);
			}
		}
	}

	evaluateMorePeers();
}

void mtt::FileTransfer::updateMeasures()
{
	std::map<PeerCommunication*, std::pair<uint64_t, uint64_t>> currentMeasure;
	auto freshPieces = downloader.popFreshPieces();

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		auto finishedUploadRequests = uploader->popHandledRequests();
		for (const auto& r : finishedUploadRequests)
		{
			auto it = activePeers.find(r.first);
			if (it != activePeers.end())
				it->second.uploaded += r.second;
		}

		for (auto& [comm, peer] : activePeers)
		{
			peer.downloaded = comm->getStream()->getReceivedDataCount();
			currentMeasure[comm] = { peer.downloaded, peer.uploaded };

			peer.downloadSpeed = 0;
			peer.uploadSpeed = 0;

			auto last = lastSpeedMeasure.find(comm);
			if (last != lastSpeedMeasure.end())
			{
				if (peer.downloaded > last->second.first)
					peer.downloadSpeed = (uint32_t)(peer.downloaded - last->second.first);
				if (peer.uploaded > last->second.second)
					peer.uploadSpeed = (uint32_t)(peer.uploaded - last->second.second);
			}

			if (comm->isEstablished())
				for (auto piece : freshPieces)
					comm->sendHave(piece);
		}
	}

	lastSpeedMeasure = std::move(currentMeasure);

	const int32_t updateMeasuresExtraInterval = 5;
	if (updateMeasuresCounter-- > 0)
		return;
	updateMeasuresCounter = updateMeasuresExtraInterval;

	if (!torrent.selectionFinished())
	{
		downloader.sortPieces(piecesAvailability);
	}
}

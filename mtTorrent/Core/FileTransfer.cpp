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
	lastSumMeasure = { downloader.downloaded, uploader->uploaded };
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
	lastSpeedMeasure.clear();
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

	downloader.handshakeFinished(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	TRANSFER_LOG("closed " << p->getStream()->getAddressName());

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
	auto inspectFunc = [this](const std::vector<PeerCommunication*>& activePeers)
	{
		const auto currentTime = mtt::CurrentTimestamp();

		const uint32_t minPeersTimeChance = 10;
		auto minTimeToEval = currentTime - minPeersTimeChance;

		const uint32_t minPeersPiecesTimeChance = 20;
		auto minTimeToReceive = currentTime - minPeersPiecesTimeChance;

		TRANSFER_LOG("evalCurrentPeers" << activePeers.size());

		if (activePeers.size() > mtt::config::getExternal().connection.maxTorrentConnections)
		{
			PeerCommunication* slowestPeer = nullptr;
			std::vector<PeerCommunication*> idleUploads;
			std::vector<PeerCommunication*> idlePeers;

			for (auto peer : activePeers)
			{
				TRANSFER_LOG(peer->getStream()->getAddress() << peer->stats.downloadSpeed << peer->stats.uploadSpeed << currentTime - peer->stats.lastActivityTime);

				if (peer->stats.connectionTime > minTimeToEval || !peer->stats.connectionTime)
				{
					continue;
				}

				if (peer->stats.lastActivityTime < minTimeToReceive)
				{
					if (peer->state.peerInterested)
						idleUploads.push_back(peer);
					else
						idlePeers.push_back(peer);

					continue;
				}

				if (!slowestPeer ||
					peer->stats.downloadSpeed < slowestPeer->stats.downloadSpeed ||
					(peer->stats.downloadSpeed == slowestPeer->stats.downloadSpeed && peer->stats.downloaded < slowestPeer->stats.downloaded))
				{
					slowestPeer = peer;
				}
			}

			const uint32_t maxProtectedUploads = 5;
			if (idleUploads.size() > maxProtectedUploads)
			{
				std::sort(idleUploads.begin(), idleUploads.end(), [](const auto& l, const auto& r)
					{ return r->stats.uploadSpeed > l->stats.uploadSpeed || (r->stats.uploadSpeed == l->stats.uploadSpeed && r->stats.uploaded > l->stats.uploaded); });

				idleUploads.resize(idleUploads.size() - maxProtectedUploads);
				for (const auto& peer : idleUploads)
				{
					TRANSFER_LOG("eval upload" << peer->getStream()->getAddressName());
					idlePeers.push_back(peer);
				}
			}

			if (idlePeers.size() > 5)
			{
				std::sort(idlePeers.begin(), idlePeers.end(),	[](const auto& l, const auto& r)
					{ return l->stats.downloaded < r->stats.downloaded; });

				idlePeers.resize(5);
			}
			else if (idlePeers.empty() && slowestPeer)
			{
				TRANSFER_LOG("eval slowest" << slowestPeer->getStream()->getAddressName());
				idlePeers.push_back(slowestPeer);
			}

			for (const auto& peer : idlePeers)
			{
				TRANSFER_LOG("eval removing" << peer->getStream()->getAddressName());
				peer->close();
			}
		}
	};
	torrent.peers->inspectConnectedPeers(inspectFunc);

	evaluateMorePeers();
}

void mtt::FileTransfer::updateMeasures()
{
	auto inspectFunc = [this](const std::vector<PeerCommunication*>& activePeers)
	{
		std::map<PeerCommunication*, std::pair<uint64_t, uint64_t>> currentMeasure;
		auto freshPieces = downloader.popFreshPieces();

		for (auto peer : activePeers)
		{
			currentMeasure[peer] = { peer->stats.downloaded, peer->stats.uploaded };

			peer->stats.downloadSpeed = 0;
			peer->stats.uploadSpeed = 0;

			auto last = lastSpeedMeasure.find(peer);
			if (last != lastSpeedMeasure.end())
			{
				if (peer->stats.downloaded > last->second.first)
					peer->stats.downloadSpeed = (uint32_t)(peer->stats.downloaded - last->second.first);
				if (peer->stats.uploaded > last->second.second)
					peer->stats.uploadSpeed = (uint32_t)(peer->stats.uploaded - last->second.second);
			}

			if (peer->isEstablished())
				for (auto piece : freshPieces)
					peer->sendHave(piece);
		}

		lastSpeedMeasure = std::move(currentMeasure);
	};
	torrent.peers->inspectConnectedPeers(inspectFunc);

	{
		downloader.downloadSpeed = 0;
		uploader->uploadSpeed = 0;

		if (downloader.downloaded > lastSumMeasure.first)
			downloader.downloadSpeed = (uint32_t)(downloader.downloaded - lastSumMeasure.first);
		if (uploader->uploaded > lastSumMeasure.second)
			uploader->uploadSpeed = (uint32_t)(uploader->uploaded - lastSumMeasure.second);

		lastSumMeasure = { downloader.downloaded, uploader->uploaded };
	}

	const int32_t updateMeasuresExtraInterval = 5;
	if (updateMeasuresCounter-- > 0)
		return;
	updateMeasuresCounter = updateMeasuresExtraInterval;

	if (!torrent.selectionFinished())
	{
		downloader.sortPieces(piecesAvailability);
	}
}

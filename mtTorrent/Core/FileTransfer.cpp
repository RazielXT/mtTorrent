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

#define TRANSFER_LOG(x) WRITE_LOG("Transfer", x)

mtt::FileTransfer::FileTransfer(TorrentPtr t) : downloader(t->infoFile.info, *this), torrent(t)
{
	uploader = std::make_shared<Uploader>(t);

	if (!ipToCountryLoaded)
	{
		ipToCountryLoaded = true;
		ipToCountry.fromFile(mtt::config::getInternal().programFolderPath);
	}

	unsavedPieceBlocksMaxSize = mtt::config::getInternal().immediateStoringBufferSize / BlockRequestMaxSize;
}

void mtt::FileTransfer::start()
{
	piecesAvailability.resize(torrent->infoFile.info.pieces.size());
	refreshSelection();

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
			uploader->refreshRequest();

			if (refreshTimer)
				refreshTimer->schedule(1);
		}
	);

	refreshTimer->schedule(1);
	evaluateCurrentPeers();
}

void mtt::FileTransfer::stop()
{
	saveLogEvents();

	{
		std::lock_guard<std::mutex> guard(peersMutex);
		activePeers.clear();
		freshPieces.clear();
	}

	torrent->peers->stop();

	auto unfinishedActive = downloader.stop();
	unFinishedPieces.insert(unFinishedPieces.end(), unfinishedActive.begin(), unfinishedActive.end());

	finishUnsavedPieceBlocks();

	if (refreshTimer)
		refreshTimer->disable();
}

void mtt::FileTransfer::clear()
{
	unFinishedPieces.clear();
}

void mtt::FileTransfer::addUnfinishedPieces(std::vector<mtt::DownloadedPieceState>& pieces)
{
	unFinishedPieces = std::move(pieces);
}

void mtt::FileTransfer::refreshSelection()
{
	piecesPriority.resize(torrent->infoFile.info.pieces.size(), Priority(0));
	std::vector<uint32_t> selected;

	for (auto& f : torrent->files.selection.files)
	{
		for (size_t i = f.info.startPieceIndex; i <= f.info.endPieceIndex; i++)
		{
			piecesPriority[i] = std::max(piecesPriority[i], f.priority);

			if (f.selected && (selected.empty() || i != selected.back()))
				selected.push_back((uint32_t)i);
		}
	}

	downloader.refreshSelection(std::move(selected));
	evaluateCurrentPeers();
}

void mtt::FileTransfer::handshakeFinished(PeerCommunication* p)
{
	TRANSFER_LOG("handshake " << p->getAddressName());
	if (!torrent->files.progress.empty())
		p->sendBitfield(torrent->files.progress.toBitfield());

	addPeer(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	TRANSFER_LOG("closed " << p->getAddressName());
	removePeer(p);
}

void mtt::FileTransfer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	TRANSFER_LOG("msg " << msg.id << " " << p->getAddressName());

	if (msg.id == Interested)
	{
		uploader->isInterested(p);
	}
	else if (msg.id == Request)
	{
		uploader->pieceRequest(p, msg.request);
	}
	else
		downloader.messageReceived(p, msg);
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
	if (idx < piecesAvailability.size())
		piecesAvailability[idx]++;
	else
	{
		auto& peerPieces = p->info.pieces.pieces;

		for (size_t i = 0; i < piecesAvailability.size(); i++)
		{
			piecesAvailability[i] += peerPieces[i] ? 1 : 0;
		}
	}

	if (!torrent->selectionFinished())
	{
		downloader.progressUpdated(p, idx);
	}
}

uint64_t& mtt::FileTransfer::getUploadSum()
{
	return uploader->uploaded;
}

uint32_t mtt::FileTransfer::getDownloadSpeed()
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.downloadSpeed;

	return sum;
}

uint32_t mtt::FileTransfer::getUploadSpeed()
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.uploadSpeed;

	return sum;
}

std::vector<mtt::DownloadedPieceState> mtt::FileTransfer::getUnfinishedPiecesState()
{
	return unFinishedPieces;
}

size_t mtt::FileTransfer::getUnfinishedPiecesDownloadSize()
{
	size_t sz = downloader.getUnfinishedPiecesDownloadSize();

	for (const auto& u : unFinishedPieces)
		sz += u.downloadedSize;

	return sz;
}

std::vector<mtt::ActivePeerInfo> mtt::FileTransfer::getPeersInfo()
{
	auto allPeers = torrent->peers->getActivePeers();

	std::vector<mtt::ActivePeerInfo> out;
	out.resize(allPeers.size());

	std::lock_guard<std::mutex> guard(peersMutex);

	uint32_t i = 0;
	for (auto& comm : allPeers)
	{
		auto& addr = comm->getAddress();
		out[i].address = addr.toString();
		out[i].country = addr.ipv6 ? "" : ipToCountry.GetCountry(swap32(*reinterpret_cast<const uint32_t*>(addr.addrBytes)));
		out[i].percentage = comm->info.pieces.getPercentage();
		out[i].client = comm->ext.state.client;
		out[i].connected = comm->isEstablished();
		out[i].choking = comm->state.peerChoking;

		for (auto& active : activePeers)
		{
			if (active.comm == comm.get())
			{
				out[i].downloadSpeed = active.downloadSpeed;
				out[i].uploadSpeed = active.uploadSpeed;
				break;
			}
		}

		i++;
	}

	return out;
}

std::vector<uint32_t> mtt::FileTransfer::getCurrentRequests()
{
	return downloader.getCurrentRequests();
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

	if (std::find_if(activePeers.begin(), activePeers.end(), [p](const mtt::ActivePeer& peer) { return peer.comm == p; }) == activePeers.end())
	{
		activePeers.push_back({ p,{} });
		activePeers.back().connectionTime = activePeers.back().lastActivityTime = (uint32_t)time(0);
		downloader.peerAdded(&activePeers.back());
	}
}

void mtt::FileTransfer::removePeer(PeerCommunication* p)
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

void mtt::FileTransfer::disconnectPeers(const std::vector<uint32_t>& positions)
{
	for (auto it = positions.rbegin(); it != positions.rend(); it++)
	{
		TRANSFER_LOG("disconnect " << activePeers[*it].comm->getAddressName());
		torrent->peers->disconnect(activePeers[*it].comm);
		activePeers.erase(activePeers.begin() + *it);
	}
}

bool mtt::FileTransfer::isWantedPiece(uint32_t idx)
{
	return torrent->files.progress.wantedPiece(idx);
}

void mtt::FileTransfer::storePieceBlock(const PieceBlock& block)
{
	std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);

 	auto buffer = getDataBuffer();
	buffer->assign(block.buffer.data, block.buffer.data + block.buffer.size);
 
 	unsavedPieceBlocks.push_back({ block.info, buffer });

	if (unsavedPieceBlocks.size() >= unsavedPieceBlocksMaxSize)
	{
		auto blocks = std::move(unsavedPieceBlocks);

		torrent->service.io.post([this, blocks]()
			{
				auto status = saveUnsavedPieceBlocks(blocks);

				if (status != Status::Success)
				{
					torrent->lastError = status;
					torrent->stop(Torrent::StopReason::Internal);
				}
			});
	}
}

std::shared_ptr<DataBuffer> mtt::FileTransfer::getDataBuffer()
{
	for (auto b : dataBuffers)
	{
		if (b->empty())
			return b;
	}

	dataBuffers.push_back(std::make_shared<DataBuffer>());
	return dataBuffers.back();
}

mtt::Status mtt::FileTransfer::saveUnsavedPieceBlocks(const std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>>& blocks)
{
	std::vector<Storage::PieceBlockRequest> blockRequests;
	blockRequests.reserve(blocks.size());

	for (auto& [info, buffer] : blocks)
		blockRequests.push_back({info.index, info.begin, buffer.get()});

	Status status = torrent->files.storage.storePieceBlocks(std::move(blockRequests));

	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);

		for (const auto& [info, buffer] : blocks)
			buffer->clear();
	}

	return status;
}

mtt::Status mtt::FileTransfer::finishUnsavedPieceBlocks()
{
	std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>> blocks;
	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);

		if (unsavedPieceBlocks.empty())
			return Status::Success;

		blocks = std::move(unsavedPieceBlocks);
	}

	Status status = saveUnsavedPieceBlocks(blocks);
	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
		dataBuffers.clear();
	}

	return status;
}

bool mtt::FileTransfer::storeUnfinishedPiece(const mtt::DownloadedPiece& piece)
{
	return torrent->files.storage.storePieceBlock(piece.index, 0, *piece.data) == Status::Success;
}

mtt::LockedPeers mtt::FileTransfer::getPeers()
{
	return { activePeers, peersMutex };
}

mtt::DownloadedPiece mtt::FileTransfer::loadUnfinishedPiece(uint32_t idx, bool loadData)
{
	for (auto& u : unFinishedPieces)
	{
		if (u.index == idx && u.downloadedSize)
		{
			mtt::DownloadedPiece piece;
			(mtt::DownloadedPieceState&)piece = std::move(u);
			u.downloadedSize = 0;

			if (loadData)
			{
				mtt::PieceBlockInfo info;
				info.begin = 0;
				info.index = idx;
				info.length = torrent->infoFile.info.getPieceSize(idx);

				piece.data = std::make_shared<DataBuffer>();

				if (torrent->files.storage.loadPieceBlock(info, *piece.data) != Status::Success)
					return {};
			}

			return std::move(piece);
		}
	}
	return {};
}

void mtt::FileTransfer::pieceFinished(const mtt::DownloadedPiece& piece)
{
	torrent->files.progress.addPiece(piece.index);

	if (piece.data)
	{
		auto torrentPtr = torrent;
		torrent->service.io.post([torrentPtr, piece]()
			{
				auto status = torrentPtr->files.storage.storePieceBlock(piece.index, 0, *piece.data);

				if (status != Status::Success)
				{
					torrentPtr->files.progress.removePiece(piece.index);
					torrentPtr->lastError = status;
					torrentPtr->stop(Torrent::StopReason::Internal);
				}
			});
	}

	{
		std::lock_guard<std::mutex> guard(peersMutex);
		freshPieces.push_back(piece.index);
	}

	if (torrent->selectionFinished())
	{
		finishUnsavedPieceBlocks();

		if (downloader.immediateMode)
			torrent->checkFiles();
	}
}

void mtt::FileTransfer::evaluateCurrentPeers()
{
	TRANSFER_LOG("evalCurrentPeers?");
	if (activePeers.size() < mtt::config::getExternal().connection.maxTorrentConnections && !torrent->selectionFinished())
		torrent->peers->connectNext(10);
}

void mtt::FileTransfer::evalCurrentPeers()
{
	TRANSFER_LOG("evalCurrentPeers");
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
			std::sort(currentUploads.begin(), currentUploads.end(), [&](const auto& l, const auto& r)
			{ return activePeers[l].uploadSpeed > activePeers[r].uploadSpeed || 
				(activePeers[l].uploadSpeed == activePeers[r].uploadSpeed && activePeers[l].uploaded > activePeers[r].uploaded); });

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

		disconnectPeers(removedPeers);
	}

	evaluateCurrentPeers();
}

void mtt::FileTransfer::updateMeasures()
{
	std::vector<std::pair<PeerCommunication*, std::pair<uint64_t, uint64_t>>> currentMeasure;

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		auto finishedUploadRequests = uploader->popHandledRequests();
		for (const auto& r : finishedUploadRequests)
		{
			if (auto peer = getActivePeer(r.first))
				peer->uploaded += r.second;
		}

		for (auto& peer : activePeers)
		{
			peer.downloaded = peer.comm->getReceivedDataCount();
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

			if (peer.comm->isEstablished())
				for (auto& piece : freshPieces)
					peer.comm->sendHave(piece);

#ifdef MTT_DIAGNOSTICS
			diagnostics.addPeer(peer.comm->diagnostics);
#endif // MTT_DIAGNOSTICS

		}

#ifdef MTT_DIAGNOSTICS
		diagnostics.flush();
#endif // MTT_DIAGNOSTICS

		freshPieces.clear();
	}

	lastSpeedMeasure = currentMeasure;

	if (!torrent->selectionFinished())
	{
		downloader.sortPriority(piecesPriority, piecesAvailability);
	}
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

			file << peerLog.addr.toString() << " Action:" << (int)peerLog.action << " dl:" << peerLog.dl / 1024.f << " up:" << peerLog.up / 1024.f << " dead:" << peerLog.activityTime << "\n";
		}

		logIndex += eval.count;
	}

	file << "\n\n";
}
#endif
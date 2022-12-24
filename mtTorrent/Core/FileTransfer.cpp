#include "FileTransfer.h"
#include "Torrent.h"
#include "Files.h"
#include "PeerCommunication.h"
#include "Peers.h"
#include "Configuration.h"
#include "AlertsManager.h"
#include "utils/FastIpToCountry.h"
#include <fstream>

FastIpToCountry ipToCountry;
bool ipToCountryLoaded = false;

#define TRANSFER_LOG(x) WRITE_LOG(x)

enum class LogEvalPeer { Ok, TooSoon, NotResponding, Upload, TooSlow };
// #define DIAGNOSTICS(x) WRITE_DIAGNOSTICS_LOG(x)

mtt::FileTransfer::FileTransfer(Torrent& t) : downloader(t.infoFile.info, *this), torrent(t)
{
	uploader = std::make_shared<Uploader>(torrent);

	if (!ipToCountryLoaded)
	{
		ipToCountryLoaded = true;
		ipToCountry.fromFile(mtt::config::getInternal().programFolderPath);
	}

	unsavedPieceBlocksMaxSize = mtt::config::getInternal().fileStoringBufferSize / BlockRequestMaxSize;

	CREATE_NAMED_LOG(Transfer, torrent.name());
}

void mtt::FileTransfer::start()
{
	piecesAvailability.resize(torrent.infoFile.info.pieces.size());
	refreshSelection();

	if (refreshTimer)
		return;

	torrent.peers->start([this](Status s, mtt::PeerSource)
		{
			if (s == Status::Success)
			{
				evaluateMorePeers();
			}
		}
	, this);

	refreshTimer = ScheduledTimer::create(torrent.service.io, [this]
		{
			evalCurrentPeers();
			updateMeasures();
			uploader->refreshRequest();

			if (refreshTimer)
				refreshTimer->schedule(std::chrono::milliseconds(1000));
		}
	);

	refreshTimer->schedule(std::chrono::milliseconds(1000));
	evaluateMorePeers();
}

void mtt::FileTransfer::stop()
{
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		activePeers.clear();
		freshPieces.clear();
	}

	torrent.peers->stop();

	auto unfinishedActive = downloader.stop();
	{
		std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);
		unFinishedPieces.insert(unFinishedPieces.end(), unfinishedActive.begin(), unfinishedActive.end());
	}

	finishUnsavedPieceBlocks();

	if (refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	uploader->stop();
}

void mtt::FileTransfer::addUnfinishedPieces(std::vector<mtt::DownloadedPiece>& pieces)
{
	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);

	unFinishedPieces.reserve(pieces.size());

	for (auto& p : pieces)
	{
		if (!torrent.files.progress.hasPiece(p.index))
			unFinishedPieces.emplace_back(std::move(p));
	}
}

void mtt::FileTransfer::clearUnfinishedPieces()
{
	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);
	unFinishedPieces.clear();
}

void mtt::FileTransfer::refreshSelection()
{
	downloader.refreshSelection(torrent.files.selection, piecesAvailability);
	evaluateMorePeers();
}

void mtt::FileTransfer::handshakeFinished(PeerCommunication* p)
{
	TRANSFER_LOG("handshake " << p->getStream()->getAddressName());
	if (!torrent.files.progress.empty())
		p->sendBitfield(torrent.files.progress.toBitfield());

	addPeer(p);
}

void mtt::FileTransfer::connectionClosed(PeerCommunication* p, int code)
{
	TRANSFER_LOG("closed " << p->getStream()->getAddressName());
	removePeer(p);
}

void mtt::FileTransfer::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	TRANSFER_LOG("msg " << msg.id << " " << p->getStream()->getAddressName());

	if (msg.id == PeerMessage::Interested)
	{
		uploader->isInterested(p);
	}
	else if (msg.id == PeerMessage::Request)
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

	if (!torrent.selectionFinished())
	{
		downloader.progressUpdated(p, idx);
	}
}

uint64_t& mtt::FileTransfer::getDownloadSum()
{
	return downloader.downloaded;
}

uint64_t& mtt::FileTransfer::getUploadSum()
{
	return uploader->uploaded;
}

uint32_t mtt::FileTransfer::getDownloadSpeed() const
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.downloadSpeed;

	return sum;
}

uint32_t mtt::FileTransfer::getUploadSpeed() const
{
	uint32_t sum = 0;

	std::lock_guard<std::mutex> guard(peersMutex);
	for (auto& peer : activePeers)
		sum += peer.uploadSpeed;

	return sum;
}

std::vector<mtt::DownloadedPiece> mtt::FileTransfer::getUnfinishedPiecesState()
{
	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);
	return unFinishedPieces;
}

size_t mtt::FileTransfer::getUnfinishedPiecesDownloadSize()
{
	size_t sz = downloader.getUnfinishedPiecesDownloadSize();

	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);

	for (const auto& u : unFinishedPieces)
		sz += u.downloadedSize;

	return sz;
}

std::map<uint32_t, uint32_t> mtt::FileTransfer::getUnfinishedPiecesDownloadSizeMap()
{
	auto map = downloader.getUnfinishedPiecesDownloadSizeMap();

	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);

	for (const auto& u : unFinishedPieces)
		map[u.index] += u.downloadedSize;

	return map;
}

std::vector<mtt::ActivePeerInfo> mtt::FileTransfer::getPeersInfo() const
{
	auto allPeers = torrent.peers->getActivePeers();

	std::vector<mtt::ActivePeerInfo> out;
	out.resize(allPeers.size());

	std::lock_guard<std::mutex> guard(peersMutex);

	uint32_t i = 0;
	for (auto& comm : allPeers)
	{
		auto addr = comm->getStream()->getAddress();
		out[i].address = addr.toString();
		out[i].country = addr.ipv6 ? "" : ipToCountry.GetCountry(swap32(*reinterpret_cast<const uint32_t*>(addr.addrBytes)));
		out[i].percentage = comm->info.pieces.getPercentage();
		out[i].client = comm->ext.state.client;
		out[i].connected = comm->isEstablished();
		out[i].choking = comm->state.peerChoking;
		out[i].flags = comm->getStream()->getFlags();

		for (auto& active : activePeers)
		{
			if (active.comm == comm.get())
			{
				out[i].downloadSpeed = active.downloadSpeed;
				out[i].uploadSpeed = active.uploadSpeed;
				out[i].requesting = !active.requestedPieces.empty();
				break;
			}
		}

		i++;
	}

	return out;
}

std::vector<uint32_t> mtt::FileTransfer::getCurrentRequests() const
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
		activePeers.back().connectionTime = activePeers.back().lastActivityTime = mtt::CurrentTimestamp();
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

	uploader->cancelRequests(p);
	evaluateMorePeers();
}

void mtt::FileTransfer::disconnectPeers(const std::vector<uint32_t>& positions)
{
	for (auto it = positions.rbegin(); it != positions.rend(); it++)
	{
		TRANSFER_LOG("disconnect " << activePeers[*it].comm->getStream()->getAddressName());
		torrent.peers->disconnect(activePeers[*it].comm);
		activePeers.erase(activePeers.begin() + *it);
	}
}

bool mtt::FileTransfer::wantsToDownload()
{
	return !torrent.checking && !torrent.selectionFinished();
}

bool mtt::FileTransfer::isMissingPiece(uint32_t idx)
{
	return torrent.files.progress.wantedPiece(idx);
}

void mtt::FileTransfer::storePieceBlock(const PieceBlock& block)
{
	std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);

 	auto buffer = getDataBuffer();
	buffer->assign(block.buffer.data, block.buffer.data + block.buffer.size);
 
	unsavedPieceBlocks.emplace_back(block.info, buffer);

	if (unsavedPieceBlocks.size() >= unsavedPieceBlocksMaxSize && wantsToDownload())
	{
		auto blocks = std::move(unsavedPieceBlocks);

		torrent.service.io.post([this, blocks]()
			{
				mtt::Status status;
				int retry = 0;
				while ((status = saveUnsavedPieceBlocks(blocks)) == mtt::Status::E_FileLockedError && retry < 5)
				{
					std::this_thread::sleep_for(retry == 0 ? std::chrono::milliseconds(100) : std::chrono::seconds(1));
					retry++;
				}

				if (status != Status::Success)
				{
					torrent.lastError = status;
					torrent.stop(Torrent::StopReason::Internal);
				}
			});
	}
}

std::shared_ptr<DataBuffer> mtt::FileTransfer::getDataBuffer()
{
	if (freeDataBuffers.empty())
		return std::make_shared<DataBuffer>();
	else
	{
		auto buffer = freeDataBuffers.back();
		freeDataBuffers.pop_back();
		return buffer;
	}
}

void mtt::FileTransfer::returnDataBuffer(std::shared_ptr<DataBuffer> buffer)
{
	buffer->clear();
	freeDataBuffers.push_back(buffer);
}

mtt::Status mtt::FileTransfer::saveUnsavedPieceBlocks(const std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>>& blocks)
{
	std::vector<Storage::PieceBlockRequest> blockRequests;
	blockRequests.reserve(blocks.size());

	for (auto& [info, buffer] : blocks)
		blockRequests.push_back({ info.index, info.begin, buffer.get() });

	Status status = torrent.files.storage.storePieceBlocks(std::move(blockRequests));

	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);

		for (auto& b : blocks)
			returnDataBuffer(b.second);
	}

	return status;
}

void mtt::FileTransfer::finishUnsavedPieceBlocks()
{
	std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>> blocks;
	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
		blocks = std::move(unsavedPieceBlocks);
	}

	if (!blocks.empty())
		saveUnsavedPieceBlocks(blocks);

	{
		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
		freeDataBuffers.clear();
	}
}

mtt::LockedPeers mtt::FileTransfer::getPeers()
{
	return { activePeers, peersMutex };
}

mtt::DownloadedPiece mtt::FileTransfer::loadUnfinishedPiece(uint32_t idx)
{
	std::lock_guard<std::mutex> guard(unFinishedPiecesMutex);

	for (auto& u : unFinishedPieces)
	{
		if (u.index == idx && u.downloadedSize)
		{
			mtt::DownloadedPiece piece;
			(mtt::DownloadedPiece&)piece = std::move(u);
			u.downloadedSize = 0;

			return piece;
		}
	}
	return {};
}

void mtt::FileTransfer::pieceFinished(const mtt::DownloadedPiece& piece)
{
	torrent.files.progress.addPiece(piece.index);

	{
		std::lock_guard<std::mutex> guard(peersMutex);
		freshPieces.push_back(piece.index);
	}

	if (torrent.selectionFinished())
	{
		finishUnsavedPieceBlocks();

		torrent.checkFiles();

		AlertsManager::Get().metadataAlert(AlertId::TorrentFinished, &torrent);
	}
}

void mtt::FileTransfer::evaluateMorePeers()
{
	TRANSFER_LOG("evaluateMorePeers");
	if (activePeers.size() < mtt::config::getExternal().connection.maxTorrentConnections && !torrent.selectionFinished() && !torrent.checking)
		torrent.service.io.post([this]()
			{
				TRANSFER_LOG("connectNext");
				torrent.peers->connectNext(10);
			});
}

void mtt::FileTransfer::evalCurrentPeers()
{
	TRANSFER_LOG("evalCurrentPeers");
	const uint32_t peersEvalInterval = 10;

	if (peersEvalCounter-- > 0)
		return;

	peersEvalCounter = peersEvalInterval;

	std::vector<uint32_t> removedPeers;
	{
		std::lock_guard<std::mutex> guard(peersMutex);

		TRANSFER_LOG("evalCurrentPeers" << activePeers.size());

		const auto currentTime = mtt::CurrentTimestamp();

		const uint32_t minPeersTimeChance = 10;
		auto minTimeToEval = currentTime - minPeersTimeChance;

		const uint32_t minPeersPiecesTimeChance = 20;
		auto minTimeToReceive = currentTime - minPeersPiecesTimeChance;

		uint32_t slowestPeer = 0;

		uint32_t maxUploads = 5;
		std::vector<uint32_t> currentUploads;

		if (activePeers.size() * 2 > mtt::config::getExternal().connection.maxTorrentConnections)
		{
			uint32_t idx = 0;
			for (const auto& peer : activePeers)
			{
				TRANSFER_LOG(idx << ":" << peer.comm->getStream()->getAddress() << peer.downloadSpeed << peer.uploadSpeed << currentTime - peer.lastActivityTime);

				if (peer.connectionTime > minTimeToEval)
				{
					TRANSFER_LOG(idx << (char)LogEvalPeer::TooSoon);
					continue;
				}

				if (peer.comm->state.peerInterested)
				{
					currentUploads.push_back(idx);
				}

				if (peer.lastActivityTime < minTimeToReceive )
				{
					TRANSFER_LOG(idx << (char)LogEvalPeer::NotResponding);
					removedPeers.push_back(idx);
					continue;
				}

				const auto& slowest = activePeers[slowestPeer];
				if (peer.downloadSpeed < slowest.downloadSpeed || (peer.downloadSpeed == slowest.downloadSpeed && peer.receivedBlocks < slowest.receivedBlocks))
				{
					slowestPeer = idx;
				}

				idx++;
			}

			if (removedPeers.size() > 5)
			{
				std::sort(removedPeers.begin(), removedPeers.end(),
					[&](uint32_t i1, uint32_t i2) { return activePeers[i1].receivedBlocks < activePeers[i2].receivedBlocks; });

				removedPeers.resize(5);
			}

			if (removedPeers.empty())
			{
				TRANSFER_LOG(slowestPeer << (char)LogEvalPeer::TooSlow);
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

					TRANSFER_LOG(idx << (char)LogEvalPeer::Upload);
				}
			}

			TRANSFER_LOG("removing" << removedPeers.size());
			disconnectPeers(removedPeers);
		}
	}

	evaluateMorePeers();
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
			peer.downloaded = peer.comm->getStream()->getReceivedDataCount();
			currentMeasure.push_back({ peer.comm, {peer.downloaded, peer.uploaded} });
			peer.downloadSpeed = 0;
			peer.uploadSpeed = 0;

			for (const auto& last : lastSpeedMeasure)
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
		}

		freshPieces.clear();
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

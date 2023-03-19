#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include "AlertsManager.h"
#include <random>

#define DL_LOG(x) WRITE_LOG(x)

mtt::Downloader::Downloader(Torrent& t) : torrent(t), storage(t)
{
	CREATE_NAMED_LOG(Downloader, torrent.name());

	storage.onPieceChecked = [this](uint32_t idx, Status s, std::shared_ptr<void> info) { pieceChecked(idx, s, *std::static_pointer_cast<RequestInfo>(info)); };
}

void mtt::Downloader::start()
{
	storage.start();
}

void mtt::Downloader::stop()
{
	DL_LOG("stop");

	std::vector<mtt::PieceState> out;
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& [idx,r] : requests)
		{
			if (r->piece.downloadedSize)
			{
				out.emplace_back(std::move(r->piece));
			}
		}
		requests.clear();
		peersRequestsInfo.clear();
	}

	unfinishedPieces.add(out);
	storage.stop();
	downloadSpeed = 0;
}

void mtt::Downloader::sortPieces(const std::vector<uint32_t>& availability)
{
	std::sort(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), [&](const PieceInfo& i1, const PieceInfo& i2)
		{
			auto missing = isMissingPiece(i1.idx);
			if (missing != isMissingPiece(i2.idx))
				return missing;

			return i1.priority > i2.priority
				|| (i1.priority == i2.priority && availability[i1.idx] < availability[i2.idx]);
		});
}

std::vector<uint32_t> mtt::Downloader::getCurrentRequests() const
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	std::vector<uint32_t> out;
	out.reserve(requests.size());

	for (const auto& it : requests)
	{
		out.push_back(it.first);
	}

	return out;
}

std::vector<mtt::PieceDownloadState> mtt::Downloader::getCurrentRequestsInfo() const
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	std::vector<PieceDownloadState> out;
	out.resize(requests.size());
	int i = 0;

	for (const auto& it : requests)
	{
		auto& info = out[i++];
		auto& r = *it.second;

		info.index = r.piece.index;
		info.blocksCount = (uint32_t)r.piece.blocksState.size();
		info.receivedBlocks = info.blocksCount - r.piece.remainingBlocks;
		info.sentRequestsCount = r.requestsCounter;
		info.activeSources = (uint32_t)r.activePeers.size();
	}

	return out;
}

std::vector<uint32_t> mtt::Downloader::popFreshPieces()
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	return std::move(freshPieces);
}

void mtt::Downloader::handshakeFinished(PeerCommunication*)
{
	//wait for bitfield/have messages for progress state
}

void mtt::Downloader::connectionClosed(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	auto it = peersRequestsInfo.find(p);
	if (it != peersRequestsInfo.end())
	{
		for (auto& r : it->second.currentRequests)
		{
			auto it = requests.find(r.idx);
			if (it != requests.end())
			{
				it->second->activePeers.erase(p);
			}
		}

		peersRequestsInfo.erase(it);
	}
}

uint64_t mtt::Downloader::getUnfinishedSelectedPiecesDownloadSize()
{
	uint64_t s = 0;

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& [idx, r] : requests)
	{
		s += r->piece.downloadedSize;
	}

	return s;
}

uint64_t mtt::Downloader::getUnfinishedPiecesDownloadSize()
{
	uint64_t s = unfinishedPieces.getDownloadSize();

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& [idx, r] : requests)
	{
		s += r->piece.downloadedSize;
	}

	return s;
}

std::map<uint32_t, uint32_t> mtt::Downloader::getUnfinishedPiecesDownloadSizeMap()
{
	std::map<uint32_t, uint32_t> map = unfinishedPieces.getDownloadSizeMap();

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& [idx,r] : requests)
	{
		map[idx] = r->piece.downloadedSize;
	}

	return map;
}

bool mtt::Downloader::isMissingPiece(uint32_t idx) const
{
	return torrent.files.progress.wantedPiece(idx);
}

void mtt::Downloader::finish()
{
	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		for (auto& [peer, info] : peersRequestsInfo)
		{
			if (peer->isEstablished() && peer->info.pieces.finished()) //seed
				peer->close();
		}
	}

	storage.stop();
	AlertsManager::Get().torrentAlert(Alerts::Id::TorrentFinished, torrent);
}

void mtt::Downloader::pieceChecked(uint32_t idx, Status s, const RequestInfo& info)
{
	DL_LOG("pieceChecked " << idx << " status " << (int)s);

	std::lock_guard<std::mutex> guard(requestsMutex);

	checkingPieces--;

	if (s == Status::Success)
		freshPieces.push_back(idx);
	else
		torrent.files.progress.removePiece(idx);

	refreshPieceSources(s, info);

	if (s == Status::Success)
	{
		if (torrent.selectionFinished() && checkingPieces == 0)
		{
			torrent.service.post([this]()
				{
					finish();
				});
		}
	}
	else if (s != Status::I_Mismatch)
	{
		torrent.service.post([this, s]()
			{
				torrent.lastError = s;
				torrent.stop(Torrent::StopReason::Internal);
			});
	}
}

void mtt::Downloader::refreshPieceSources(Status status, const RequestInfo& info)
{
	if (status == Status::I_Mismatch)
		GroupIdx++;

	for (auto peer : info.sources)
	{
		auto it = peersRequestsInfo.find(peer);
		if (it != peersRequestsInfo.end())
		{
			auto& peerInfo = it->second;
			if (status == Status::Success)
			{
				peerInfo.suspiciousGroup = 0;
				peerInfo.suspicion = 0;
			}
			else if (status == Status::I_Mismatch)
			{
				peerInfo.suspicion++;

				if (info.sources.size() == 1 || peerInfo.suspicion > 2)
				{
					if (peerInfo.suspiciousGroup)
						for (auto& [p, i] : peersRequestsInfo)
							if (i.suspiciousGroup == peerInfo.suspiciousGroup)
								i.suspiciousGroup = 0;

					torrent.peers->disconnect(peer, true);
				}
				else
				{
					peerInfo.suspiciousGroup = GroupIdx;
				}
			}
		}
	}
}

void mtt::Downloader::pieceBlockReceived(PieceBlock& block, PeerCommunication* source)
{
	DL_LOG("Receive block idx " << block.info.index << " begin " << block.info.begin);
	bool finished = false;

	std::lock_guard<std::mutex> guard(requestsMutex);

	auto peer = addPeerBlockResponse(block, source);
	if (!peer)
	{
		duplicatedDataSum += block.info.length;
		DL_LOG("Unwanted block " << block.info.index << ", begin" << block.info.begin << "total " << duplicatedDataSum);
		return;
	}

	auto it = requests.find(block.info.index);
	if (it != requests.end())
	{
		auto& r = it->second;

		if (!r->piece.addBlock(block))
		{
			duplicatedDataSum += block.info.length;
			DL_LOG("Duplicate block " << block.info.index << ", begin" << block.info.begin << "total " << duplicatedDataSum);
		}
		else
		{
			if (peer->suspiciousGroup)
				r->sourceGroups.insert(peer->suspiciousGroup);

			r->sources.insert(source);
			storage.storePieceBlock(block);

			if (r->piece.remainingBlocks == 0)
			{
				finished = true;

				if (!isMissingPiece(r->piece.index))
				{
					duplicatedDataSum += r->piece.downloadedSize;
					DL_LOG("Duplicate piece " << block.info.index << "total " << duplicatedDataSum);
				}
				else
				{
					DL_LOG("Request finished, piece " << block.info.index);
					storage.checkPiece(block.info.index, std::move(r));
					checkingPieces++;
				}

				torrent.files.progress.addPiece(block.info.index);
				requests.erase(it);
			}
		}
	}

	downloaded += block.buffer.size;

	if (finished)
	{
		for (auto& [p, info] : peersRequestsInfo)
			for (auto it = info.currentRequests.begin(); it != info.currentRequests.end(); it++)
			{
				if (it->idx == block.info.index)
				{
					DL_LOG("Erase peer request " << block.info.index);
					info.currentRequests.erase(it);
					break;
				}
			}
	}

	evaluateNextRequests(*peer, source);
}

static uint32_t blockIdx(uint32_t begin)
{
	return begin / BlockMaxSize;
}

mtt::Downloader::PeerRequestsInfo* mtt::Downloader::addPeerBlockResponse(PieceBlock& block, PeerCommunication* source)
{
	auto it = peersRequestsInfo.find(source);
	if (it != peersRequestsInfo.end())
	{
		auto& info = it->second;
		info.received();

		for (auto& request : info.currentRequests)
		{
			if (request.idx == block.info.index)
			{
				if (request.blocks[blockIdx(block.info.begin)])
				{
					request.blocks[blockIdx(block.info.begin)] = false;
					request.activeBlocksCount--;
				}
				return &info;
			}
		}

		for (auto it = info.requestedPieces.rbegin(); it != info.requestedPieces.rend(); it++)
			if (*it == block.info.index)
				return &info;
	}

	return nullptr;
}

void mtt::Downloader::unchokePeer(PeerCommunication* peer)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	auto it = peersRequestsInfo.find(peer);
	if (it != peersRequestsInfo.end())
	{
		auto& info = it->second;
		for (auto& r : info.currentRequests)
		{
			r.blocks.assign(r.blocks.size(), false);
			r.activeBlocksCount = 0;
		}

		evaluateNextRequests(info, peer);
	}
}

void mtt::Downloader::messageReceived(PeerCommunication* comm, PeerMessage& msg)
{
	if (torrent.selectionFinished())
	{
		if (comm->isEstablished() && comm->info.pieces.finished()) //seed
			comm->close();
		return;
	}

	if (msg.id == PeerMessage::Piece)
	{
		pieceBlockReceived(msg.piece, comm);
	}
	else if (msg.id == PeerMessage::Unchoke)
	{
		unchokePeer(comm);
	}
	else if (msg.id == PeerMessage::Have || msg.id == PeerMessage::Bitfield)
	{
		if (msg.id == PeerMessage::Have && !isMissingPiece(msg.havePieceIndex))
			return;

		std::lock_guard<std::mutex> guard(requestsMutex);

		if (msg.id == PeerMessage::Bitfield && !hasWantedPieces(comm))
			return;

		evaluateNextRequests(peersRequestsInfo[comm], comm);
	}
}

void mtt::Downloader::refreshSelection(const DownloadSelection& s, const std::vector<uint32_t>& availability)
{
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		sortedSelectedPieces.clear();

		for (size_t i = 0; i < torrent.infoFile.info.files.size(); i++)
		{
			const auto& file = torrent.infoFile.info.files[i];
			const auto& selection = s[i];

			for (uint32_t idx = file.startPieceIndex; idx <= file.endPieceIndex; idx++)
			{
				if (selection.selected)
				{
					if (sortedSelectedPieces.empty() || idx != sortedSelectedPieces.back().idx)
						sortedSelectedPieces.push_back({ idx, selection.priority });
				}
			}
		}

		std::shuffle(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), std::minstd_rand{ (uint32_t)mtt::CurrentTimestamp() });

		for (auto it = requests.begin(); it != requests.end();)
		{
			auto idx = it->first;

			if (!isMissingPiece(idx))
			{
				for (auto& [peer, info] : peersRequestsInfo)
				{
					auto reqIt = std::find_if(info.currentRequests.begin(), info.currentRequests.end(), [idx](const PeerRequestsInfo::RequestedPiece& req) { return req.idx == idx; });
					if (reqIt != info.currentRequests.end())
						info.currentRequests.erase(reqIt);
				}

				it = requests.erase(it);
			}
			else
			{
				it++;
			}
		}

		sortPieces(availability);
	}

	if (!torrent.selectionFinished())
	{
		storage.start();

		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& [peer, info] : peersRequestsInfo)
			evaluateNextRequests(info, peer);
	}
}

void mtt::Downloader::evaluateNextRequests(PeerRequestsInfo& info, PeerCommunication* peer)
{
	if (peer->state.peerChoking)
	{
		if (!peer->state.amInterested && hasWantedPieces(peer))
			peer->setInterested(true);

		return;
	}

	const uint32_t MinPendingPeerRequests = 5;
	const uint32_t MaxPendingPeerRequests = 100;
	auto maxRequests = std::clamp((uint32_t)info.deltaReceive, MinPendingPeerRequests, MaxPendingPeerRequests);

	uint32_t currentRequests = 0;
	for (const auto& r : info.currentRequests)
		currentRequests += r.activeBlocksCount;

	DL_LOG("Current requests" << currentRequests << "pieces count" << info.currentRequests.size());

	if (currentRequests < maxRequests)
	{
		for (auto& r : info.currentRequests)
		{
			currentRequests += sendPieceRequests(peer, r, getRequest(r.idx), maxRequests - currentRequests);

			if (currentRequests >= maxRequests)
				break;
		}
	}

	while (currentRequests < maxRequests)
	{
		auto request = getBestNextRequest(info, peer);

		if (!request)
			break;

		DL_LOG("Next piece " << request->piece.index);

		info.currentRequests.push_back({ request->piece.index });
		info.currentRequests.back().blocks.resize(request->piece.blocksState.size());
		info.requestedPieces.push_back(request->piece.index);
		request->activePeers.insert(peer);

		currentRequests += sendPieceRequests(peer, info.currentRequests.back(), *request, maxRequests - currentRequests);
	}

	DL_LOG("New requests" << currentRequests << "maxRequests" << maxRequests);
}

mtt::Downloader::RequestInfo& mtt::Downloader::getRequest(uint32_t idx)
{
	auto& request = requests[idx];
	if (!request)
	{
		DL_LOG("Request add idx " << idx);
		request = std::make_shared<RequestInfo>();
		request->piece = unfinishedPieces.load(idx);

		if (request->piece.blocksState.empty())
			request->piece.init(idx, torrent.infoFile.info.getPieceBlocksCount(idx));
	}

	return *request;
}

mtt::Downloader::RequestInfo* mtt::Downloader::getBestNextRequest(PeerRequestsInfo& info, PeerCommunication* peer)
{
	if (sortedSelectedPieces.empty())
		return nullptr;

	auto firstPriority = sortedSelectedPieces.front().priority;
	auto lastPriority = firstPriority;

	if (fastCheck)
		for (auto [idx, priority] : sortedSelectedPieces)
		{
			if (firstPriority != priority)
				break;

			if (isMissingPiece(idx) && peer->info.pieces.hasPiece(idx))
			{
				auto it = requests.find(idx);
				if (it == requests.end() || it->second->activePeers.empty())
				{
					DL_LOG("fast getBestNextPiece idx" << idx);
					return &getRequest(idx);
				}
			}
		}

	if (fastCheck)
	{
		DL_LOG("fail fast getBestNextPiece");
		fastCheck = false;
	}

	std::vector<RequestInfo*> possibleShareRequests;
	possibleShareRequests.reserve(10);

	auto getBestSharedRequest = [&]() -> RequestInfo*
	{
		RequestInfo* bestSharedRequest = nullptr;

		for (auto request : possibleShareRequests)
		{
			if (info.suspiciousGroup && request->sourceGroups.find(info.suspiciousGroup) != request->sourceGroups.end())
				continue;

			if (!bestSharedRequest ||
				bestSharedRequest->activePeers.size() > request->activePeers.size() ||
				(bestSharedRequest->activePeers.size() == request->activePeers.size() && bestSharedRequest->requestsCounter > request->requestsCounter))
				bestSharedRequest = request;
		}

		if (bestSharedRequest)
			DL_LOG("getBestSharedRequest idx " << bestSharedRequest->piece.index);

		return bestSharedRequest;
	};

	for (auto [idx, priority] : sortedSelectedPieces)
	{
		if (!isMissingPiece(idx))
			continue;

		if (peer->info.pieces.hasPiece(idx))
		{
			if (lastPriority > priority)
			{
				DL_LOG("lastPriority overflow");

				if (auto r = getBestSharedRequest())
					return r;
			}

			lastPriority = priority;

			auto it = requests.find(idx);
			if (it != requests.end() && !it->second->activePeers.empty())
			{
				bool alreadyRequested = false;
				for (auto& r : info.currentRequests)
				{
					if (r.idx == idx)
						alreadyRequested = true;
				}
				if (!alreadyRequested)
					possibleShareRequests.push_back(it->second.get());
			}
			else
			{
				DL_LOG("getBestNextPiece idx" << idx);
				fastCheck = (lastPriority == firstPriority);
				return &getRequest(idx);
			}
		}
	}

	if (auto r = getBestSharedRequest())
		return r;

	DL_LOG("getBestNextPiece idx none");

	return nullptr;
}

uint32_t mtt::Downloader::sendPieceRequests(PeerCommunication* peer, PeerRequestsInfo::RequestedPiece& request, RequestInfo& info, uint32_t max)
{
	uint32_t count = 0;
	auto blocksCount = (uint16_t)info.piece.blocksState.size();

	uint16_t nextBlock = info.requestsCounter % blocksCount;
	for (uint32_t i = 0; i < blocksCount; i++)
	{
		if (info.piece.blocksState.empty() || info.piece.blocksState[nextBlock] == 0)
		{
			if (!request.blocks[nextBlock])
			{
				auto info = torrent.infoFile.info.getPieceBlockInfo(request.idx, nextBlock);
				DL_LOG("Send block request " << info.index << info.begin);
				peer->requestPieceBlock(info);
				request.blocks[nextBlock] = true;
				request.activeBlocksCount++;
				count++;
			}
		}

		nextBlock = (nextBlock + 1) % blocksCount;

		if (count == max)
			break;
	}
	info.requestsCounter += (uint16_t)count;

	return count;
}

bool mtt::Downloader::hasWantedPieces(PeerCommunication* peer)
{
	if (peer->info.pieces.pieces.size() != torrent.infoFile.info.pieces.size())
		return false;

	for (auto [idx,priority] : sortedSelectedPieces)
	{
		if (isMissingPiece(idx) && peer->info.pieces.hasPiece(idx))
			return true;
	}

	return false;
}

void mtt::Downloader::PeerRequestsInfo::received()
{
	auto now = Torrent::TimeClock::now();
	auto currentDelta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReceive);
	lastReceive = now;

	if (currentDelta > std::chrono::seconds(1))
		deltaReceive = 0;
	else if (deltaReceive > 0)
		deltaReceive = (1000 - currentDelta.count()) / 1000.f * deltaReceive;

	deltaReceive += 1;
}

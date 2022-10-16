#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include <numeric>
#include <random>

#define DL_LOG(x) WRITE_LOG(x)

const uint32_t MinPendingPeerRequests = 5;
const uint32_t MaxPendingPeerRequests = 50;
const float DlSpeedPerMoreRequest = 40*1024.f;

mtt::Downloader::Downloader(const TorrentInfo& info, DownloaderClient& c) : torrentInfo(info), client(c)
{
	CREATE_NAMED_LOG(Downloader, info.name);
}

std::vector<mtt::DownloadedPiece> mtt::Downloader::stop()
{
	DL_LOG("stop");

	std::vector<mtt::DownloadedPiece> out;
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& r : requests)
		{
			if (r->piece.downloadedSize)
			{
				piecesState[r->pieceIdx].request = nullptr;
				out.emplace_back(std::move(r->piece));
			}
		}
		requests.clear();
	}

	return out;
}

void mtt::Downloader::sortPieces(const std::vector<uint32_t>& availability)
{
	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	std::sort(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), [&](uint32_t i1, uint32_t i2)
		{
			if (piecesState[i1].missing != piecesState[i2].missing)
				return piecesState[i1].missing;

			return piecesState[i1].priority > piecesState[i2].priority 
				|| (piecesState[i1].priority == piecesState[i2].priority && availability[i1] < availability[i2]);
		});
}

std::vector<uint32_t> mtt::Downloader::getCurrentRequests() const
{
	std::vector<uint32_t> out;

	std::lock_guard<std::mutex> guard(requestsMutex);
	out.reserve(requests.size());

	for (const auto& r : requests)
	{
		out.push_back(r->pieceIdx);
	}

	return out;
}

size_t mtt::Downloader::getUnfinishedPiecesDownloadSize()
{
	size_t s = 0;

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& r : requests)
	{
		s += r->piece.downloadedSize;
	}

	return s;
}

std::map<uint32_t, uint32_t> mtt::Downloader::getUnfinishedPiecesDownloadSizeMap()
{
	std::map<uint32_t, uint32_t> map;

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& r : requests)
	{
		map[r->pieceIdx] = r->piece.downloadedSize;
	}

	return map;
}

void mtt::Downloader::peerAdded(ActivePeer* peer)
{
	evaluateNextRequests(peer);
}

void mtt::Downloader::pieceBlockReceived(PieceBlock& block, PeerCommunication* source)
{
	bool finished = false;

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		DL_LOG("Receive block idx " << block.info.index << " begin " << block.info.begin);

		for (auto it = requests.begin(); it != requests.end(); it++)
		{
			auto& r = *it;
			if (r->pieceIdx == block.info.index)
			{
				if (r->piece.blocksState.empty())
					r->piece.init(r->pieceIdx, r->blocksCount);

				if (!r->piece.addBlock(block))
				{
					duplicatedDataSum += block.info.length;
					DL_LOG("Duplicate block " << block.info.index << ", begin" << block.info.begin << "total " << duplicatedDataSum);
					break;
				}

				client.storePieceBlock(block);

				if (r->piece.remainingBlocks == 0)
				{
					finished = true;

					if (!client.isMissingPiece(r->pieceIdx))
					{
						duplicatedDataSum += r->piece.downloadedSize;
						DL_LOG("Duplicate piece " << block.info.index << "total " << duplicatedDataSum);
					}

					DL_LOG("Request finished, piece " << block.info.index);
					client.pieceFinished(std::move(r->piece));

					std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

					auto& state = piecesState[block.info.index];
					state.missing = false;
					state.request = nullptr;

					requests.erase(it);
				}

				break;
			}
		}
	}

	downloaded += block.buffer.size;

	PieceStatus status = finished ? PieceStatus::Finished : PieceStatus::Ok;

	auto peers = client.getPeers();
	refreshPeerBlockRequests(peers.get(), block, status, source);
}

static uint32_t blockIdx(uint32_t begin)
{
	return begin / BlockRequestMaxSize;
}

void mtt::Downloader::refreshPeerBlockRequests(std::vector<mtt::ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source)
{
	for (auto& peer : peers)
	{
		for (auto it = peer.requestedPieces.begin(); it != peer.requestedPieces.end(); it++)
		{
			if (it->idx == block.info.index)
			{
				if (status == PieceStatus::Finished)
					peer.requestedPieces.erase(it);
				else
					it->blocks[blockIdx(block.info.begin)] = false;

				break;
			}
		}

		if (peer.comm == source)
		{
			if (status == PieceStatus::Invalid)
				peer.invalidPieces++;
			else
				peer.receivedBlocks++;

			peer.lastActivityTime = (uint32_t)time(0);

			evaluateNextRequests(&peer);
		}
	}
}

void mtt::Downloader::unchokePeer(ActivePeer* peer)
{
	auto currentTime = (uint32_t)time(0);
	if (currentTime - peer->lastActivityTime > 5)
	{
		for (auto& p : peer->requestedPieces)
			p.blocks.resize(p.blocks.size(), false);
	}

	peer->lastActivityTime = currentTime;

	evaluateNextRequests(peer);
}

void mtt::Downloader::messageReceived(PeerCommunication* comm, PeerMessage& msg)
{
	if (msg.id == PeerMessage::Piece)
	{
		pieceBlockReceived(msg.piece, comm);
	}
	else if (msg.id == PeerMessage::Unchoke)
	{
		auto peers = client.getPeers();

		if (auto peer = peers.get(comm))
			unchokePeer(peer);
	}
}

void mtt::Downloader::progressUpdated(PeerCommunication* p, uint32_t idx)
{
	auto peers = client.getPeers();

	if (auto peer = peers.get(p))
		evaluateNextRequests(peer);
}

void mtt::Downloader::refreshSelection(const DownloadSelection& s, const std::vector<uint32_t>& availability)
{
	auto peers = client.getPeers();

	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		std::lock_guard<std::mutex> guard2(sortedSelectedPiecesMutex);

		piecesState.clear();
		piecesState.resize(torrentInfo.pieces.size());

		sortedSelectedPieces.clear();

		for (size_t i = 0; i < torrentInfo.files.size(); i++)
		{
			const auto& file = torrentInfo.files[i];
			const auto& selection = s[i];

			for (uint32_t idx = file.startPieceIndex; idx <= file.endPieceIndex; idx++)
			{
				if (selection.selected)
				{
					piecesState[idx].priority = std::max(piecesState[idx].priority, selection.priority);
					piecesState[idx].missing = client.isMissingPiece(idx);

					if (sortedSelectedPieces.empty() || idx != sortedSelectedPieces.back())
						sortedSelectedPieces.push_back(idx);
				}
			}
		}

		std::shuffle(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), std::minstd_rand{ (uint32_t)time(0) });

		for (auto it = requests.begin(); it != requests.end();)
		{
			auto& r = *it;

			if (!piecesState[r->pieceIdx].missing)
			{
				for (auto& p : peers.get())
				{
					auto rIt = std::find_if(p.requestedPieces.begin(), p.requestedPieces.end(), [&](const ActivePeer::RequestedPiece& req) { return req.idx == r->pieceIdx; });
					if (rIt != p.requestedPieces.end())
						p.requestedPieces.erase(rIt);
				}

				it = requests.erase(it);
			}
			else
			{
				piecesState[r->pieceIdx].request = it->get();
				it++;
			}
		}
	}

	sortPieces(availability);

	for (auto& p : peers.get())
		evaluateNextRequests(&p);
}

void mtt::Downloader::evaluateNextRequests(ActivePeer* peer)
{
	if (!client.wantsToDownload())
		return;

	if (peer->comm->state.peerChoking)
	{
		if (!peer->comm->info.pieces.empty() && !peer->comm->state.amInterested && hasWantedPieces(peer))
			peer->comm->setInterested(true);

		return;
	}

	auto maxRequests = MinPendingPeerRequests;
	if (peer->receivedBlocks > 30)
	{
		if (maxRequests < (uint32_t)(peer->downloadSpeed / DlSpeedPerMoreRequest))
			maxRequests = std::min(MaxPendingPeerRequests, maxRequests + 1);
		else
			maxRequests = std::max(MinPendingPeerRequests, maxRequests - 1);
	}

	uint32_t currentRequests = 0;
	for (auto& r : peer->requestedPieces)
		for (auto b : r.blocks)
			if (b)
				currentRequests++;

	DL_LOG("Current requests" << currentRequests << "pieces count" << peer->requestedPieces.size());

	std::lock_guard<std::mutex> guard(requestsMutex);

	if (currentRequests < maxRequests)
	{
		for (auto& r : peer->requestedPieces)
		{
			currentRequests += sendPieceRequests(peer, &r, getRequest(r.idx), maxRequests - currentRequests);

			if (currentRequests >= maxRequests)
				break;
		}
	}

	while (currentRequests < maxRequests)
	{
		auto request = getBestNextRequest(peer);

		if (!request)
			break;

		DL_LOG("Next piece " << request->pieceIdx);

		peer->requestedPieces.push_back({ request->pieceIdx });
		peer->requestedPieces.back().blocks.resize(request->blocksCount);

		currentRequests += sendPieceRequests(peer, &peer->requestedPieces.back(), request, maxRequests - currentRequests);
	}

	DL_LOG("New requests" << currentRequests << "maxRequests" << maxRequests << "receivedBlocks" << peer->receivedBlocks << "dl" << peer->downloadSpeed);
}

mtt::Downloader::RequestInfo* mtt::Downloader::getRequest(uint32_t idx)
{
	for (auto& r : requests)
	{
		if (r->pieceIdx == idx)
			return r.get();
	}

	return addRequest(idx);
}

mtt::Downloader::RequestInfo* mtt::Downloader::addRequest(uint32_t idx)
{
	DL_LOG("Request add idx" << idx);
	auto request = std::make_shared<RequestInfo>();
	request->pieceIdx = idx;
	request->blocksCount = (uint16_t)torrentInfo.getPieceBlocksCount(idx);
	request->piece = client.loadUnfinishedPiece(request->pieceIdx);

	auto ptr = request.get();
	piecesState[idx].request = ptr;
	requests.emplace_back(std::move(request));

	return ptr;
}

mtt::Downloader::RequestInfo* mtt::Downloader::getBestNextRequest(ActivePeer* peer)
{
	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	auto firstPriority = sortedSelectedPieces.empty() ? Priority(0) : piecesState[sortedSelectedPieces.front()].priority;
	auto lastPriority = firstPriority;

	if (fastCheck)
		for (auto idx : sortedSelectedPieces)
		{
			auto& info = piecesState[idx];
			if (firstPriority != info.priority)
				break;

			if (info.missing && !info.request && peer->comm->info.pieces.hasPiece(idx))
			{
				DL_LOG("fast getBestNextPiece idx" << idx);
				return addRequest(idx);
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

		for (auto r : possibleShareRequests)
		{
			if (!bestSharedRequest || bestSharedRequest->blockRequestsCount > r->blockRequestsCount)
				bestSharedRequest = r;
		}

		if (bestSharedRequest)
		{
			DL_LOG("getBestSharedRequest idx " << bestSharedRequest->pieceIdx);
			return bestSharedRequest;
		}

		return nullptr;
	};

	for (auto idx : sortedSelectedPieces)
	{
		auto& info = piecesState[idx];
		if (!info.missing)
			continue;

		if (peer->comm->info.pieces.hasPiece(idx))
		{
			if (lastPriority > info.priority)
			{
				DL_LOG("lastPriority overflow");

				if (auto r = getBestSharedRequest())
					return r;
			}

			lastPriority = info.priority;

			if (info.request)
			{
				bool alreadyRequested = false;
				for (const auto& r : peer->requestedPieces)
				{
					if (r.idx == idx)
					{
						alreadyRequested = true;
						break;
					}
				}

				if (!alreadyRequested)
					possibleShareRequests.push_back(info.request);
			}
			else
			{
				DL_LOG("getBestNextPiece idx" << idx);
				fastCheck = (lastPriority == firstPriority);
				return addRequest(idx);
			}
		}
	}
	
	if (auto r = getBestSharedRequest())
		return r;

	DL_LOG("getBestNextPiece idx none");

	return nullptr;
}

uint32_t mtt::Downloader::sendPieceRequests(ActivePeer* peer, ActivePeer::RequestedPiece* request, RequestInfo* r, uint32_t max)
{
	uint32_t count = 0;

	uint16_t nextBlock = r->blockRequestsCount % r->blocksCount;
	for (uint32_t i = 0; i < r->blocksCount; i++)
	{
		if (r->piece.blocksState.empty() || r->piece.blocksState[nextBlock] == 0)
		{
			if (!request->blocks[nextBlock])
			{
				auto info = torrentInfo.getPieceBlockInfo(request->idx, nextBlock);
				DL_LOG("Send block request " << info.index << info.begin);
				peer->comm->requestPieceBlock(info);
				request->blocks[nextBlock] = true;
				count++;
			}
		}

		nextBlock = (nextBlock + 1) % r->blocksCount;

		if (count == max)
			break;
	}
	r->blockRequestsCount += (uint16_t)count;

	return count;
}

bool mtt::Downloader::hasWantedPieces(ActivePeer* peer)
{
	if (peer->comm->info.pieces.pieces.size() != torrentInfo.pieces.size())
		return false;

	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	for (auto idx : sortedSelectedPieces)
	{
		if (piecesState[idx].missing && peer->comm->info.pieces.hasPiece(idx))
			return true;
	}

	return false;
}

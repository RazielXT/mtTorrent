#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include <numeric>
#include <random>

#define DL_LOG(x) WRITE_LOG(x)

const size_t MaxPreparedPieces = 10;
const uint32_t MinPendingPeerRequests = 5;
const uint32_t MaxPendingPeerRequests = 50;
const float DlSpeedPerMoreRequest = 40*1024.f;

mtt::Downloader::Downloader(const TorrentInfo& info, DownloaderClient& c) : torrentInfo(info), client(c)
{
	CREATE_NAMED_LOG(Downloader, info.name);
}

std::vector<mtt::DownloadedPiece> mtt::Downloader::stop()
{
	std::vector<mtt::DownloadedPiece> out;
	std::vector<RequestInfo> stoppedRequests;
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		stoppedRequests = std::move(requests);
		requests.clear();
	}

	for (auto& r : stoppedRequests)
	{
		if (r.piece.downloadedSize)
		{
			out.emplace_back(std::move(r.piece));
		}
	}

	return out;
}

void mtt::Downloader::sortPriority(const std::vector<uint32_t>& availability)
{
	std::lock_guard<std::mutex> guard(priorityMutex);

	std::sort(selectedPieces.begin(), selectedPieces.end(),
		[&](uint32_t i1, uint32_t i2) { return piecesPriority[i1] > piecesPriority[i2] || (piecesPriority[i1] == piecesPriority[i2] && availability[i1] < availability[i2]); });
}

std::vector<uint32_t> mtt::Downloader::getCurrentRequests() const
{
	std::vector<uint32_t> out;

	std::lock_guard<std::mutex> guard(requestsMutex);
	for (auto& r : requests)
	{
		out.push_back(r.pieceIdx);
	}

	return out;
}

size_t mtt::Downloader::getUnfinishedPiecesDownloadSize()
{
	size_t s = 0;

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto& r : requests)
	{
		s += r.piece.downloadedSize;
	}

	return s;
}

std::map<uint32_t, uint32_t> mtt::Downloader::getUnfinishedPiecesDownloadSizeMap()
{
	std::map<uint32_t, uint32_t> map;

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto& r : requests)
	{
		map[r.pieceIdx] = r.piece.downloadedSize;
	}

	return map;
}

void mtt::Downloader::peerAdded(ActivePeer* peer)
{
	evaluateNextRequests(peer);
}

void mtt::Downloader::pieceBlockReceived(PieceBlock& block, PeerCommunication* source)
{
	bool valid = true;
	bool finished = false;

	DL_LOG("Receive block idx " << block.info.index << " begin " << block.info.begin);

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto it = requests.begin(); it != requests.end(); it++)
		{
			auto& r = *it;
			if (r.pieceIdx == block.info.index)
			{
				r.lastActivityTime = (uint32_t)time(0);

				if (r.piece.blocksState.empty())
					r.piece.init(r.pieceIdx, r.blocksCount);

				if (!r.piece.addBlock(block))
				{
					duplicatedDataSum += block.info.length;
					break;
				}

				client.storePieceBlock(block);

				if (r.piece.remainingBlocks == 0)
				{
					finished = true;

					if (!client.isWantedPiece(r.pieceIdx))
						duplicatedDataSum += r.piece.downloadedSize;

					if (valid)
					{
						DL_LOG("Request finished, piece " << block.info.index);
						client.pieceFinished(std::move(r.piece));
					}
					else
						duplicatedDataSum += r.piece.downloadedSize;

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

void mtt::Downloader::refreshPeerBlockRequests(std::vector<mtt::ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source)
{
	for (auto& peer : peers)
	{
		if (peer.comm == source)
		{
			if (status == PieceStatus::Invalid)
				peer.invalidPieces++;
			else
				peer.receivedBlocks++;

			peer.lastActivityTime = (uint32_t)time(0);
		}

		for (auto it = peer.requestedPieces.begin(); it != peer.requestedPieces.end(); it++)
		{
			if (it->idx == block.info.index)
			{
				if (status == PieceStatus::Finished)
					peer.requestedPieces.erase(it);
				else
				{
					for (auto it2 = it->blocks.begin(); it2 != it->blocks.end(); it2++)
					{
						if (*it2 == block.info.begin)
						{
							it->blocks.erase(it2);
							break;
						}
					}
				}

				evaluateNextRequests(&peer);
				break;
			}
		}
	}
}

void mtt::Downloader::unchokePeer(ActivePeer* peer)
{
	auto currentTime = (uint32_t)time(0);
	if (currentTime - peer->lastActivityTime > 5)
	{
		for (auto& p : peer->requestedPieces)
			p.blocks.clear();
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

void mtt::Downloader::refreshSelection(const DownloadSelection& s)
{
	auto peers = client.getPeers();

	{
		std::lock_guard<std::mutex> guard(priorityMutex);

		piecesPriority.resize(torrentInfo.pieces.size(), Priority(0));
		selectedPieces.clear();

		for (size_t i = 0; i < torrentInfo.files.size(); i++)
		{
			const auto& file = torrentInfo.files[i];
			const auto& selection = s[i];

			for (size_t i = file.startPieceIndex; i <= file.endPieceIndex; i++)
			{
				piecesPriority[i] = std::max(piecesPriority[i], selection.priority);

				if (selection.selected && (selectedPieces.empty() || i != selectedPieces.back()))
					selectedPieces.push_back((uint32_t)i);
			}
		}

		auto rng = std::minstd_rand{ (uint32_t)time(0) };
		std::shuffle(selectedPieces.begin(), selectedPieces.end(), rng);
	}

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto it = requests.begin(); it != requests.end();)
		{
			auto& r = *it;

			if (!client.isWantedPiece(r.pieceIdx))
			{
				for (auto& p : peers.get())
				{
					auto rIt = std::find_if(p.requestedPieces.begin(), p.requestedPieces.end(), [&](const ActivePeer::RequestedPiece& req) { return req.idx == r.pieceIdx; });
					if (rIt != p.requestedPieces.end())
						p.requestedPieces.erase(rIt);
				}

				it = requests.erase(it);
			}
			else
				it++;
		}
	}

	for (auto& p : peers.get())
		evaluateNextRequests(&p);
}

void mtt::Downloader::evaluateNextRequests(ActivePeer* peer)
{
	if (client.isFinished())
		return;

	if (peer->comm->state.peerChoking)
	{
		if (!peer->comm->info.pieces.empty() && !peer->comm->state.amInterested && hasWantedPieces(peer))
			peer->comm->setInterested(true);

		return;
	}

	if (peer->requestedPieces.empty())
	{
		auto pieces = getBestNextPieces(peer);

		if (!pieces.empty())
		{
			for (auto& piece : pieces)
			{
				peer->requestedPieces.push_back(ActivePeer::RequestedPiece{ piece });
			}
		}
	}

	if (!peer->requestedPieces.empty())
		sendPieceRequests(peer);
}

std::vector<uint32_t> mtt::Downloader::getBestNextPieces(ActivePeer* p)
{
	std::vector<uint32_t> out;
	std::vector<uint32_t> requestedElsewhere;
	auto currentTime = (uint32_t)time(0);
	Priority lastPriority = Priority::Low;

	std::lock_guard<std::mutex> guard(priorityMutex);

	for (auto idx : selectedPieces)
	{
		if (client.isWantedPiece(idx) && p->comm->info.pieces.hasPiece(idx))
		{
			bool alreadyRequested = false;

			for (auto& r : p->requestedPieces)
			{
				if (r.idx == idx)
				{
					alreadyRequested = true;
					break;
				}
			}

			if (!alreadyRequested)
			{
				if (lastPriority > piecesPriority[idx] && !requestedElsewhere.empty())
				{
					auto addCount = std::min(MaxPreparedPieces - out.size(), requestedElsewhere.size());
					for (size_t i = 0; i < addCount; i++)
					{
						out.push_back(requestedElsewhere[i]);
					}
					requestedElsewhere.clear();
				}

				std::lock_guard<std::mutex> guard(requestsMutex);
				for (auto& r : requests)
				{
					if (r.pieceIdx == idx)
					{
						if (r.lastActivityTime + 5 > currentTime)
						{
							if (requestedElsewhere.size() + out.size() < MaxPreparedPieces)
								requestedElsewhere.push_back(idx);

							alreadyRequested = true;
						}

						break;
					}
				}

				if (!alreadyRequested)
					out.push_back(idx);
			}

			if (out.size() >= MaxPreparedPieces)
				break;

			lastPriority = piecesPriority[idx];
		}
	}

	if (out.size() < MaxPreparedPieces && !requestedElsewhere.empty())
	{
		auto addCount = std::min(MaxPreparedPieces - out.size(), requestedElsewhere.size());
		for (size_t i = 0; i < addCount; i++)
		{
			out.push_back(requestedElsewhere[i]);
		}
	}

	return out;
}

void mtt::Downloader::sendPieceRequests(ActivePeer* p)
{
	uint32_t count = 0;
	for (auto& piece : p->requestedPieces)
	{
		count += (uint32_t)piece.blocks.size();
	}

	DL_LOG("Current requests" << count << "pieces count" << p->requestedPieces.size());

	static auto maxRequests = MinPendingPeerRequests;
	if (p->receivedBlocks > 30)
	{
		if (maxRequests < (uint32_t)(p->downloadSpeed / DlSpeedPerMoreRequest))
			maxRequests = std::min(MaxPendingPeerRequests, maxRequests + 1);
		else
			maxRequests = std::max(MinPendingPeerRequests, maxRequests - 1);
	}

	if (count < maxRequests)
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& currentPiece : p->requestedPieces)
		{
			RequestInfo* request = nullptr;
			for (auto& r : requests)
			{
				if (r.pieceIdx == currentPiece.idx)
				{
					request = &r;
					break;
				}
			}

			if (!request)
			{
				DL_LOG("Request add idx" << currentPiece.idx);
				requests.emplace_back(RequestInfo());
				request = &requests.back();
				request->pieceIdx = currentPiece.idx;
				request->blocksCount = (uint16_t)torrentInfo.getPieceBlocksCount(currentPiece.idx);
				request->piece = client.loadUnfinishedPiece(request->pieceIdx);
				request->lastActivityTime = (uint32_t)time(0);
			}

			count += sendPieceRequests(p, &currentPiece, request, maxRequests - count);
			if (count >= maxRequests)
				break;
		}

		DL_LOG("New requests" << count << "maxRequests" << maxRequests << "receivedBlocks" << p->receivedBlocks << "dl" << p->downloadSpeed);
	}
}

uint32_t mtt::Downloader::sendPieceRequests(ActivePeer* peer, ActivePeer::RequestedPiece* request, RequestInfo* r, uint32_t max)
{
	uint32_t count = 0;

	uint16_t nextBlock = r->nextBlockRequestIdx;
	for (uint32_t i = 0; i < r->blocksCount; i++)
	{
		if (r->piece.blocksState.empty() || r->piece.blocksState[nextBlock] == 0)
		{
			if (std::find(request->blocks.begin(), request->blocks.end(), nextBlock*BlockRequestMaxSize) == request->blocks.end())
			{
				auto info = torrentInfo.getPieceBlockInfo(request->idx, nextBlock);
				DL_LOG("Send block request " << info.index << info.begin);
				request->blocks.push_back(info.begin);
				peer->comm->requestPieceBlock(info);
				count++;
			}
		}

		nextBlock = (nextBlock + 1) % r->blocksCount;

		if (count == max)
			break;
	}
	r->nextBlockRequestIdx = nextBlock;

	return count;
}

bool mtt::Downloader::hasWantedPieces(ActivePeer* p)
{
	std::lock_guard<std::mutex> guard(priorityMutex);

	if (p->comm->info.pieces.pieces.size() != torrentInfo.pieces.size())
		return false;

	for (auto i : selectedPieces)
	{
		if (p->comm->info.pieces.hasPiece(i) && client.isWantedPiece(i))
			return true;
	}

	return false;
}

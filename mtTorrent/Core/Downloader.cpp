#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include "utils/SHA.h"
#include <numeric>
#include <random>

#define DL_LOG(x) WRITE_LOG(LogTypeDownload, x)

const size_t MaxPreparedPieces = 10;
const uint32_t MinPendingPeerRequestsBeforeNext = 5;
const uint32_t MaxPendingPeerRequests = 10;
const uint32_t MaxPendingPeerRequestsToSpeedRatio = (1024*1024);

mtt::Downloader::Downloader(TorrentInfo& info, DownloaderClient& c) : torrentInfo(info), client(c)
{
	immediateMode = info.pieceSize >= mtt::config::getInternal().minPieceSizeForImmediateStoring;
}

std::vector<mtt::DownloadedPieceState> mtt::Downloader::stop()
{
	std::vector<mtt::DownloadedPieceState> out;
	std::vector<RequestInfo> stoppedRequests;
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		stoppedRequests = std::move(requests);
		requests.clear();
	}

	for (auto& r : stoppedRequests)
	{
		if (r.piece && r.piece->downloadedSize)
		{
			if (immediateMode || client.storeUnfinishedPiece(r.piece))
				out.emplace_back(std::move(*r.piece));
		}
	}

	return std::move(out);
}

void mtt::Downloader::sortPriority(const std::vector<Priority>& priority, const std::vector<uint32_t>& availability)
{
	std::lock_guard<std::mutex> guard(priorityMutex);

	sortedPieces = selectedPieces;

	std::sort(sortedPieces.begin(), sortedPieces.end(),
		[&availability](uint32_t i1, uint32_t i2) {return availability[i1] < availability[i2]; });

	std::sort(sortedPieces.begin(), sortedPieces.end(),
		[&priority](uint32_t i1, uint32_t i2) { return priority[i1] > priority[i2]; });
}

std::vector<uint32_t> mtt::Downloader::getCurrentRequests()
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
		if (r.piece)
			s += r.piece->downloadedSize;
	}

	return s;
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

				if (!r.piece)
				{
					r.piece = std::make_shared<DownloadedPiece>();
					r.piece->init(r.pieceIdx, immediateMode ? 0 : torrentInfo.getPieceSize(r.pieceIdx), r.blocksCount);
				}

				r.piece->addBlock(block);

				if (immediateMode)
					client.storePieceBlock(block);

				if (r.piece->remainingBlocks == 0)
				{
					finished = true;

					if (!immediateMode)
						valid = r.piece->isValid(torrentInfo.pieces[r.pieceIdx].hash);

					if (valid)
					{
						DL_LOG("Request finished, piece " << block.info.index);
						client.pieceFinished(std::move(r.piece));
					}

					requests.erase(it);
				}

				break;
			}
		}
	}

	PieceStatus status = finished ? (valid ? PieceStatus::Finished : PieceStatus::Invalid) : PieceStatus::Ok;

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
	if (msg.id == Piece)
	{
		pieceBlockReceived(msg.piece, comm);
	}
	else if (msg.id == Unchoke)
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

void mtt::Downloader::refreshSelection(std::vector<uint32_t> selected)
{
	auto peers = client.getPeers();

	{
		std::lock_guard<std::mutex> guard(priorityMutex);

		selectedPieces = std::move(selected);

		auto rng = std::minstd_rand{ (uint32_t)time(0) };
		std::shuffle(selectedPieces.begin(), selectedPieces.end(), rng);

		sortedPieces = selectedPieces;
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

	std::lock_guard<std::mutex> guard(priorityMutex);

	for (auto idx : sortedPieces)
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
				std::lock_guard<std::mutex> guard(requestsMutex);
				for (auto& r : requests)
				{
					if (r.pieceIdx == idx)
					{
						if (r.lastActivityTime + 10 > currentTime)
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

	if (count < MinPendingPeerRequestsBeforeNext)
	{
		auto maxRequests = MaxPendingPeerRequests;
		if (p->receivedBlocks > 30)
		{
			maxRequests = MaxPendingPeerRequests * std::max(1U, p->downloadSpeed / MaxPendingPeerRequestsToSpeedRatio);
		}

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
				DL_LOG("Request add " << currentPiece.idx);
				requests.push_back(RequestInfo());
				request = &requests.back();
				request->pieceIdx = currentPiece.idx;
				request->blocksCount = (uint16_t)torrentInfo.getPieceBlocksCount(currentPiece.idx);
				request->piece = client.loadUnfinishedPiece(request->pieceIdx, !immediateMode);
				request->lastActivityTime = (uint32_t)time(0);
			}

			count += sendPieceRequests(p, &currentPiece, request, maxRequests - count);

			if(count >= maxRequests)
				break;
		}
	}
}

uint32_t mtt::Downloader::sendPieceRequests(ActivePeer* peer, ActivePeer::RequestedPiece* request, RequestInfo* r, uint32_t max)
{
	uint32_t count = 0;

	uint16_t nextBlock = r->nextBlockRequestIdx;
	for (uint32_t i = 0; i < r->blocksCount; i++)
	{
		if (!r->piece || r->piece->blocksState[nextBlock] == 0)
		{
			if (std::find(request->blocks.begin(), request->blocks.end(), nextBlock*BlockRequestMaxSize) == request->blocks.end())
			{
				auto info = torrentInfo.getPieceBlockInfo(request->idx, nextBlock);
				DL_LOG("Send block request " << info.index << "-" << info.begin);
				request->blocks.push_back(info.begin);
				peer->comm->requestPieceBlock(info);
				count++;
			}
		}

		nextBlock = (nextBlock + 1) % r->blocksCount;

		if(count == max)
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

	for (auto i : sortedPieces)
	{
		if (p->comm->info.pieces.hasPiece(i) && client.isWantedPiece(i))
			return true;
	}

	return false;
}

bool mtt::DownloadedPiece::isValid(const uint8_t* expectedHash)
{
	uint8_t hash[SHA_DIGEST_LENGTH];
	_SHA1((const uint8_t*)data.data(), data.size(), hash);

	return memcmp(hash, expectedHash, SHA_DIGEST_LENGTH) == 0;
}

void mtt::DownloadedPiece::init(uint32_t idx, uint32_t pieceSize, uint32_t blocksCount)
{
	if (pieceSize)
		data.resize(pieceSize);

	remainingBlocks = blocksCount;
	blocksState.assign(remainingBlocks, 0);
	index = idx;
}

void mtt::DownloadedPiece::addBlock(const mtt::PieceBlock& block)
{
	auto blockIdx = (block.info.begin + 1) / BlockRequestMaxSize;

	if (blockIdx < blocksState.size() && blocksState[blockIdx] == 0)
	{
		if (!data.empty())
			memcpy(&data[0] + block.info.begin, block.buffer.data, block.info.length);

		blocksState[blockIdx] = 1;
		remainingBlocks--;
		downloadedSize += block.info.length;
	}
}

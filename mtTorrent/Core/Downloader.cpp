#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "utils/HexEncoding.h"
#include "Configuration.h"
#include <numeric>
#include <random>

#define DL_LOG(x) WRITE_LOG(LogTypeDownload, x)

const size_t MaxPreparedPieces = 10;
const uint32_t MinPendingPeerRequestsBeforeNext = 5;
const uint32_t MaxPendingPeerRequests = 10;
const uint32_t MaxPendingPeerRequestsToSpeedRatio = (1024*1024);

mtt::Downloader::Downloader(TorrentPtr t)
{
	torrent = t;
}

void mtt::Downloader::reset()
{
	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		requests.clear();
	}

	{
		std::lock_guard<std::mutex> guard(priorityMutex);

		piecesPriority.resize(torrent->infoFile.info.pieces.size());
		std::iota(piecesPriority.begin(), piecesPriority.end(), 0);
	}
}

void mtt::Downloader::sortPriorityByAvailability(std::vector<uint32_t>& availability)
{
	std::lock_guard<std::mutex> guard(priorityMutex);

	std::iota(piecesPriority.begin(), piecesPriority.end(), 0);

	std::sort(piecesPriority.begin(), piecesPriority.end(),
		[&availability](uint32_t i1, uint32_t i2) {return availability[i1] < availability[i2]; });

	uint32_t lastIdx = 0;
	auto rng = std::default_random_engine{};
	for (uint32_t i = 1; i <= (uint32_t)piecesPriority.size(); i++)
	{
		if (i == (uint32_t)piecesPriority.size() || availability[piecesPriority[i]] != availability[piecesPriority[i - 1]])
		{
			if (lastIdx + 1 < i)
			{
				std::shuffle(piecesPriority.begin() + lastIdx, piecesPriority.begin() + i, rng);
			}

			lastIdx = i;
		}
	}
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

uint32_t mtt::Downloader::getCurrentRequestsCount()
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	return (uint32_t) requests.size();
}

mtt::Downloader::PieceStatus mtt::Downloader::pieceBlockReceived(PieceBlock& block)
{
	bool valid = true;
	bool finished = false;

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& r : requests)
		{
			if (r.pieceIdx == block.info.index)
			{
				if (!r.piece)
				{
					r.piece = std::make_shared<DownloadedPiece>();
					r.piece->init(r.pieceIdx, torrent->infoFile.info.getPieceSize(r.pieceIdx), r.blocksCount);
				}

				r.piece->addBlock(block);

				if (r.piece->remainingBlocks == 0)
				{
					finished = true;
					valid = pieceFinished(&r);
				}
				break;
			}
		}
	}

	LOG_APPEND("receive " << block.info.index << " " << block.info.begin);

	if (valid && finished && torrent->selectionFinished())
		onFinish();

	if (finished)
		return valid ? Finished : Invalid;
	else
		return Ok;
}

void mtt::Downloader::removeBlockRequests(std::vector<mtt::ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source)
{
	for (auto& peer : peers)
	{
		if (peer.comm == source)
		{
			if (status == Invalid)
				peer.invalidPieces++;
			else
				peer.receivedBlocks++;
		}

		for (auto it = peer.requestedPieces.begin(); it != peer.requestedPieces.end(); it++)
		{
			if (it->idx == block.info.index)
			{
				if (status == Finished)
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
	if ((uint32_t)time(0) - peer->lastActivityTime > 5)
	{
		for (auto& p : peer->requestedPieces)
		{
			p.blocks.clear();
		}
	}

	evaluateNextRequests(peer);
}

void mtt::Downloader::evaluateNextRequests(ActivePeer* peer)
{
	if (peer->comm->state.peerChoking)
	{
		if (!peer->comm->info.pieces.empty() && !peer->comm->state.amInterested)
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
				peer->requestedPieces.push_back(ActivePeer::RequestedPiece{ piece,{} });
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

	std::lock_guard<std::mutex> guard(priorityMutex);

	for(auto idx : piecesPriority)
	{
		if (p->comm->info.pieces.pieces[idx])
		{
			if (torrent->files.progress.wantedPiece(idx))
			{
				bool alreadyRequested = false;
				for (auto& r : p->requestedPieces)
				{
					if (r.idx == idx)
						alreadyRequested = true;
				}

				if (!alreadyRequested)
				{
					std::lock_guard<std::mutex> guard(requestsMutex);
					for (auto& r : requests)
					{
						if (r.pieceIdx == idx)
						{
							if (requestedElsewhere.size() + out.size() < MaxPreparedPieces)
								requestedElsewhere.push_back(idx);

							alreadyRequested = true;
							break;
						}
					}

					if(!alreadyRequested)
						out.push_back(idx);
				}
			}
		}

		if(out.size() >= MaxPreparedPieces)
			break;
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
				request->blocksCount = (uint16_t)torrent->infoFile.info.getPieceBlocksCount(currentPiece.idx);
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
		if (!r->piece || r->piece->blocksTodo[nextBlock] == 0)
		{
			if (std::find(request->blocks.begin(), request->blocks.end(), nextBlock*BlockRequestMaxSize) == request->blocks.end())
			{
				auto info = torrent->infoFile.info.getPieceBlockInfo(request->idx, nextBlock);
				DL_LOG("Send block request " << info.index << "-" << info.begin);
				request->blocks.push_back(info.begin);
				peer->comm->requestPieceBlock(info);
				count++;

				LOG_APPEND("request " << info.index << " " << info.begin << " " << peer->comm->getAddressName());
			}
		}

		nextBlock = (nextBlock + 1) % r->blocksCount;

		if(count == max)
			break;
	}
	r->nextBlockRequestIdx = nextBlock;

	return count;
}

bool mtt::Downloader::pieceFinished(RequestInfo* r)
{
	DL_LOG("Finished piece " << r->pieceIdx);

	bool valid = r->piece->isValid(torrent->infoFile.info.pieces[r->pieceIdx].hash);

	if (valid)
		torrent->files.addPiece(*r->piece.get());

	for (auto it = requests.begin(); it != requests.end(); it++)
	{
		if (it->pieceIdx == r->pieceIdx)
		{
			DL_LOG("Request rem " << r->pieceIdx);
			requests.erase(it);
			break;
		}
	}

	return valid;
}

void mtt::Downloader::onFinish()
{
	torrent->files.storage.flush();
}

#include "Downloader.h"
#include "Peers.h"
#include "Torrent.h"
#include "utils/HexEncoding.h"
#include "Configuration.h"

#define DL_LOG(x) WRITE_LOG(LogTypeDownload, x)

const size_t MaxPreparedPieces = 10;
const uint32_t MinPendingPeerRequestsBeforeNext = 5;
const uint32_t MaxPendingPeerRequests = 10;
const uint32_t MaxPendingPeerRequestsToSpeedRatio = (1024*1024);

mtt::Downloader::Downloader(TorrentPtr t)
{
	torrent = t;
}

void mtt::Downloader::start()
{
	torrent->peers->start([this](Status s, mtt::PeerSource)
	{
		if (s == Status::Success)
		{
			evaluateCurrentPeers();
		}
	}
	, this);
}

void mtt::Downloader::stop()
{
	torrent->peers->stop();
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		activePeers.clear();
	}
	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		requests.clear();
	}
}

void mtt::Downloader::handshakeFinished(PeerCommunication* p)
{
	DL_LOG("Added peer " << hexToString(p->info.id, 20));
	addPeer(p);
}

void mtt::Downloader::connectionClosed(PeerCommunication* p, int)
{
	{
		std::lock_guard<std::mutex> guard(peersMutex);
		DL_LOG("Removed peer " << hexToString(p->info.id, 20));
		removePeer(p);
	}

	evaluateCurrentPeers();
}

void mtt::Downloader::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	if (msg.id == Piece)
	{
		pieceBlockReceived(p, msg.piece);
	}
	else if (msg.id == Unchoke)
	{
		evaluateNextRequests(p);
	}
}

void mtt::Downloader::extHandshakeFinished(PeerCommunication*)
{
}

void mtt::Downloader::metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&)
{
}

void mtt::Downloader::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::Downloader::progressUpdated(PeerCommunication* p)
{
	evaluateNextRequests(p);
}

mtt::Downloader::ActivePeer* mtt::Downloader::getActivePeer(PeerCommunication* p)
{
	for (auto& peer : activePeers)
		if (peer.comm == p)
			return &peer;

	return nullptr;
}

void mtt::Downloader::addPeer(PeerCommunication* p)
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

	if(!found)
		activePeers.push_back({ p,{} });
	
	evaluateNextRequests(&activePeers.back());
}

void mtt::Downloader::evaluateNextRequests(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(peersMutex);
	if(auto peer = getActivePeer(p))
		evaluateNextRequests(peer);
}

void mtt::Downloader::removePeer(PeerCommunication* p)
{
	for (auto it = activePeers.begin(); it != activePeers.end(); it++)
	{
		if (it->comm == p)
		{
			activePeers.erase(it);
			break;
		}
	}
}

void mtt::Downloader::evaluateNextRequests(ActivePeer* peer)
{
	if (peer->comm->state.peerChoking)
	{
		if (!peer->comm->state.amInterested)
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

	sendPieceRequests(peer);
}

void mtt::Downloader::evaluateCurrentPeers()
{
	if(activePeers.size() < mtt::config::external.maxTorrentConnections)
		torrent->peers->connectNext(10);
}

std::vector<uint32_t> mtt::Downloader::getBestNextPieces(ActivePeer* p)
{
	std::vector<uint32_t> out;
	std::vector<uint32_t> requestedElsewhere;

	for(uint32_t idx = 0; idx < p->comm->info.pieces.pieces.size(); idx++)
	{
		if (p->comm->info.pieces.pieces[idx])
		{
			if (torrent->files.progress.pieces[idx] == 0)
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
		if (p->finishedBlocks > 30)
		{
			maxRequests = MaxPendingPeerRequests * std::max(1U, torrent->peers->analyzer.getDownloadSpeed(p->comm) / MaxPendingPeerRequestsToSpeedRatio);
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
				//DL_LOG("Request add " << currentPiece.idx);
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
				//DL_LOG("Send block request " << info.index << "-" << info.begin);
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

void mtt::Downloader::pieceBlockReceived(PeerCommunication* p, PieceBlock& block)
{
	bool valid = true;
	bool finished = false;
	//DL_LOG("Received block " << block.info.index << "-" << block.info.begin);

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

	{
		std::lock_guard<std::mutex> guard(peersMutex);

		for (auto& peer : activePeers)
		{
			if (finished)
				for (auto it = peer.requestedPieces.begin(); it != peer.requestedPieces.end(); it++)
				{
					if (it->idx == block.info.index)
					{
						//DL_LOG("Finished removing request " << it->idx);
						peer.invalidPieces += !valid;
						peer.requestedPieces.erase(it);
						evaluateNextRequests(&peer);
						break;
					}
				}
			else if (peer.comm == p)
			{
				peer.finishedBlocks++;

				for (auto it = peer.requestedPieces.begin(); it != peer.requestedPieces.end(); it++)
				{
					if (it->idx == block.info.index)
					{
						for (auto it2 = it->blocks.begin(); it2 != it->blocks.end(); it2++)
						{
							if (*it2 == block.info.begin)
							{
								//DL_LOG("Finished removing block " << it->idx << "-" << block.info.begin);
								it->blocks.erase(it2);
								break;
							}
						}
						break;
					}
				}

				evaluateNextRequests(&peer);
			}
		}
	}
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
			//DL_LOG("Request rem " << r->pieceIdx);
			requests.erase(it);
			break;
		}
	}

	return valid;
}

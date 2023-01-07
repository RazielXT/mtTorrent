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

		for (auto& r : requests)
		{
			if (r->piece.downloadedSize)
			{
				piecesDlState[r->piece.index].request = false;
				out.emplace_back(std::move(r->piece));
			}
		}
		requests.clear();
		peersRequestsInfo.clear();
	}

	unfinishedPieces.add(out);
	storage.stop();
}

void mtt::Downloader::sortPieces(const std::vector<uint32_t>& availability)
{
	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	std::sort(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), [&](uint32_t i1, uint32_t i2)
		{
			if (piecesDlState[i1].missing != piecesDlState[i2].missing)
				return piecesDlState[i1].missing;

			return piecesDlState[i1].priority > piecesDlState[i2].priority
				|| (piecesDlState[i1].priority == piecesDlState[i2].priority && availability[i1] < availability[i2]);
		});
}

std::vector<uint32_t> mtt::Downloader::getCurrentRequests() const
{
	std::vector<uint32_t> out;

	std::lock_guard<std::mutex> guard(requestsMutex);
	out.reserve(requests.size());

	for (const auto& r : requests)
	{
		out.push_back(r->piece.index);
	}

	return out;
}

std::vector<uint32_t> mtt::Downloader::popFreshPieces()
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	return std::move(freshPieces);
}

void mtt::Downloader::handshakeFinished(PeerCommunication* p)
{
	if (hasWantedPieces(p))
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		peersRequestsInfo.push_back({ p });
		evaluateNextRequests(&peersRequestsInfo.back());
	}
}

void mtt::Downloader::connectionClosed(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	auto it = std::find(peersRequestsInfo.begin(), peersRequestsInfo.end(), p);
	if (it != peersRequestsInfo.end())
		peersRequestsInfo.erase(it);
}

size_t mtt::Downloader::getUnfinishedPiecesDownloadSize()
{
	size_t s = unfinishedPieces.getDownloadSize();

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& r : requests)
	{
		s += r->piece.downloadedSize;
	}

	return s;
}

std::map<uint32_t, uint32_t> mtt::Downloader::getUnfinishedPiecesDownloadSizeMap()
{
	std::map<uint32_t, uint32_t> map = unfinishedPieces.getDownloadSizeMap();

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (const auto& r : requests)
	{
		map[r->piece.index] = r->piece.downloadedSize;
	}

	return map;
}

bool mtt::Downloader::isMissingPiece(uint32_t idx)
{
	return torrent.files.progress.wantedPiece(idx);
}

void mtt::Downloader::finish()
{
	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		for (auto& p : peersRequestsInfo)
		{
			if (p.comm->isEstablished() && p.comm->info.pieces.finished()) //seed
				p.comm->close();
		}
	}

	storage.stop();
	AlertsManager::Get().torrentAlert(Alerts::Id::TorrentFinished, torrent);
}

void mtt::Downloader::pieceChecked(uint32_t idx, Status s, const RequestInfo& info)
{
	DL_LOG("pieceChecked " << idx << " status " << (int)s);

	if (s == Status::Success)
	{
		torrent.files.progress.addPiece(idx);

		if (torrent.selectionFinished())
		{
			torrent.service.post([this]()
				{
					finish();
				});
		}
	}
	else
	{
		std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

		auto& state = piecesDlState[idx];
		state.missing = true;
	}

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		if (s == Status::Success)
			freshPieces.push_back(idx);

		refreshPieceSources(info.piece.index, s, info.sources);
	}

	if (s != Status::Success && s != Status::I_Mismatch)
	{
		torrent.service.post([this, s]()
			{
				torrent.lastError = s;
				torrent.stop(Torrent::StopReason::Internal);
			});
	}
}

void mtt::Downloader::refreshPieceSources(uint32_t idx, Status status, const std::set<PeerCommunication*>& sources)
{
	for (auto peer : sources)
	{
		auto it = std::find(peersRequestsInfo.begin(), peersRequestsInfo.end(), peer);
		if (it != peersRequestsInfo.end())
		{
			if (status == Status::Success)
			{
				it->sharedSuspicion.clear();
			}
			else if (status == Status::I_Mismatch)
			{
				if (sources.size() == 1)
				{
					for (auto notSuspicious : it->sharedSuspicion)
					{
						if (notSuspicious != peer)
						{
							auto it2 = std::find(peersRequestsInfo.begin(), peersRequestsInfo.end(), notSuspicious);
							if (it2 != peersRequestsInfo.end())
							{
								it2->sharedSuspicion.clear();

								for (auto& r : requests)
								{
									if (r->dontShare && r->sources.find(peer) != r->sources.end())
										r->dontShare = false;
								}
							}
						}
					}
					torrent.peers->disconnect(peer, true);
				}
				else
				{
					it->sharedSuspicion.insert(sources.begin(), sources.end());
				}
			}
		}
	}
}

void mtt::Downloader::pieceBlockReceived(PieceBlock& block, PeerCommunication* source)
{
	DL_LOG("Receive block idx " << block.info.index << " begin " << block.info.begin);
	bool finished = false;
	bool needed = false;

	std::lock_guard<std::mutex> guard(requestsMutex);

	auto peer = addPeerBlockResponse(block, source);
	if (!peer)
	{
		duplicatedDataSum += block.info.length;
		DL_LOG("Duplicate block " << block.info.index << ", begin" << block.info.begin << "total " << duplicatedDataSum);
		return;
	}

	for (auto it = requests.begin(); it != requests.end(); it++)
	{
		auto& r = *it;
		if (r->piece.index == block.info.index)
		{
			needed = true;

			if (!r->piece.addBlock(block))
			{
				duplicatedDataSum += block.info.length;
				DL_LOG("Duplicate block " << block.info.index << ", begin" << block.info.begin << "total " << duplicatedDataSum);
				break;
			}

			r->sources.insert(peer->comm);
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
				}

				std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

				auto& state = piecesDlState[block.info.index];
				state.missing = false;
				state.request = false;

				requests.erase(it);
			}

			break;
		}
	}

	downloaded += block.buffer.size;

	if (finished || !needed)
	{
		for (auto it = peer->requestedPieces.begin(); it != peer->requestedPieces.end(); it++)
		{
			if (it->idx == block.info.index)
			{
				peer->requestedPieces.erase(it);
				break;
			}
		}
	}

	evaluateNextRequests(peer);
}

static uint32_t blockIdx(uint32_t begin)
{
	return begin / BlockMaxSize;
}

mtt::Downloader::PeerRequestsInfo* mtt::Downloader::addPeerBlockResponse(PieceBlock& block, PeerCommunication* source)
{
	for (auto& peer : peersRequestsInfo)
	{
		if (peer.comm == source)
		{
			for (auto& requestedPiece : peer.requestedPieces)
			{
				if (requestedPiece.idx == block.info.index)
				{
					if (requestedPiece.blocks[blockIdx(block.info.begin)])
					{
						requestedPiece.blocks[blockIdx(block.info.begin)] = false;
						requestedPiece.activeBlocksCount--;
					}
					break;
				}
			}

			peer.received();
			return &peer;
		}
	}

	return nullptr;
}

void mtt::Downloader::unchokePeer(PeerRequestsInfo* peer)
{
	for (auto& r : peer->requestedPieces)
	{
		r.blocks.assign(r.blocks.size(), false);
		r.activeBlocksCount = 0;
	}

	evaluateNextRequests(peer);
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
		std::lock_guard<std::mutex> guard(requestsMutex);

		auto it = std::find(peersRequestsInfo.begin(), peersRequestsInfo.end(), comm);
		if (it != peersRequestsInfo.end())
			unchokePeer(&*it);
	}
	else if (msg.id == PeerMessage::Have || msg.id == PeerMessage::Bitfield)
	{
		if (msg.id == PeerMessage::Have && !isMissingPiece(msg.havePieceIndex))
			return;

		std::lock_guard<std::mutex> guard(requestsMutex);

		auto it = std::find(peersRequestsInfo.begin(), peersRequestsInfo.end(), comm);
		if (it != peersRequestsInfo.end())
			evaluateNextRequests(&*it);
		else
		{
			if (msg.id != PeerMessage::Bitfield || hasWantedPieces(comm))
			{
				peersRequestsInfo.push_back({ comm });
				evaluateNextRequests(&peersRequestsInfo.back());
			}
		}
	}
}

void mtt::Downloader::refreshSelection(const DownloadSelection& s, const std::vector<uint32_t>& availability)
{
	{
		std::lock_guard<std::mutex> guard(requestsMutex);
		std::lock_guard<std::mutex> guard2(sortedSelectedPiecesMutex);

		piecesDlState.assign(torrent.infoFile.info.pieces.size(), {});

		sortedSelectedPieces.clear();

		for (size_t i = 0; i < torrent.infoFile.info.files.size(); i++)
		{
			const auto& file = torrent.infoFile.info.files[i];
			const auto& selection = s[i];

			for (uint32_t idx = file.startPieceIndex; idx <= file.endPieceIndex; idx++)
			{
				if (selection.selected)
				{
					piecesDlState[idx].priority = std::max(piecesDlState[idx].priority, selection.priority);
					piecesDlState[idx].missing = isMissingPiece(idx);

					if (sortedSelectedPieces.empty() || idx != sortedSelectedPieces.back())
						sortedSelectedPieces.push_back(idx);
				}
			}
		}

		std::shuffle(sortedSelectedPieces.begin(), sortedSelectedPieces.end(), std::minstd_rand{ mtt::CurrentTimestamp() });

		for (auto it = requests.begin(); it != requests.end();)
		{
			auto& r = *it;

			if (!piecesDlState[r->piece.index].missing)
			{
				for (auto& p : peersRequestsInfo)
				{
					auto rIt = std::find_if(p.requestedPieces.begin(), p.requestedPieces.end(), [&](const PeerRequestsInfo::RequestedPiece& req) { return req.idx == r->piece.index; });
					if (rIt != p.requestedPieces.end())
						p.requestedPieces.erase(rIt);
				}

				it = requests.erase(it);
			}
			else
			{
				piecesDlState[r->piece.index].request = true;
				it++;
			}
		}
	}

	sortPieces(availability);

	if (!torrent.selectionFinished())
	{
		storage.start();

		for (auto& p : peersRequestsInfo)
			evaluateNextRequests(&p);
	}
}

void mtt::Downloader::evaluateNextRequests(PeerRequestsInfo* peer)
{
	if (peer->comm->state.peerChoking)
	{
		if (!peer->comm->info.pieces.empty() && !peer->comm->state.amInterested && hasWantedPieces(peer->comm))
			peer->comm->setInterested(true);

		return;
	}

	const uint32_t MinPendingPeerRequests = 5;
	const uint32_t MaxPendingPeerRequests = 100;
	auto maxRequests = std::clamp((uint32_t)peer->deltaReceive, MinPendingPeerRequests, MaxPendingPeerRequests);

	uint32_t currentRequests = 0;
	for (const auto& r : peer->requestedPieces)
		currentRequests += r.activeBlocksCount;

	DL_LOG("Current requests" << currentRequests << "pieces count" << peer->requestedPieces.size());

	if (currentRequests < maxRequests)
	{
		for (auto& r : peer->requestedPieces)
		{
			currentRequests += sendPieceRequests(peer->comm, &r, getRequest(r.idx), maxRequests - currentRequests);

			if (currentRequests >= maxRequests)
				break;
		}
	}

	while (currentRequests < maxRequests)
	{
		auto request = getBestNextRequest(peer);

		if (!request)
			break;

		if (peer->suspicious())
			request->dontShare = true;

		DL_LOG("Next piece " << request->piece.index);

		peer->requestedPieces.push_back({ request->piece.index });
		peer->requestedPieces.back().blocks.resize(request->piece.blocksState.size());

		currentRequests += sendPieceRequests(peer->comm, &peer->requestedPieces.back(), request, maxRequests - currentRequests);
	}

	DL_LOG("New requests" << currentRequests << "maxRequests" << maxRequests);
}

mtt::Downloader::RequestInfo* mtt::Downloader::getRequest(uint32_t idx)
{
	for (auto& r : requests)
	{
		if (r->piece.index == idx)
			return r.get();
	}

	return addRequest(idx);
}

mtt::Downloader::RequestInfo* mtt::Downloader::addRequest(uint32_t idx)
{
	DL_LOG("Request add idx" << idx);
	auto request = std::make_shared<RequestInfo>();
	request->piece = unfinishedPieces.load(idx);

	if (request->piece.blocksState.empty())
		request->piece.init(idx, torrent.infoFile.info.getPieceBlocksCount(idx));

	auto ptr = request.get();
	piecesDlState[idx].request = true;
	requests.emplace_back(std::move(request));

	return ptr;
}

mtt::Downloader::RequestInfo* mtt::Downloader::getBestNextRequest(PeerRequestsInfo* peer)
{
	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	auto firstPriority = sortedSelectedPieces.empty() ? Priority(0) : piecesDlState[sortedSelectedPieces.front()].priority;
	auto lastPriority = firstPriority;

	if (fastCheck)
		for (auto idx : sortedSelectedPieces)
		{
			auto& info = piecesDlState[idx];
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

	std::vector<uint32_t> possibleShareRequests;
	possibleShareRequests.reserve(10);

	auto getBestSharedRequest = [&]() -> RequestInfo*
	{
		RequestInfo* bestSharedRequest = nullptr;

		for (auto idx : possibleShareRequests)
		{
			RequestInfo* requestInfo = nullptr;
			for (const auto& info : requests)
				if (info->piece.index == idx && !info->dontShare)
					requestInfo = info.get();

			if (requestInfo)
				if (!bestSharedRequest || bestSharedRequest->activeRequestsCount > requestInfo->activeRequestsCount)
					bestSharedRequest = requestInfo;
		}

		if (bestSharedRequest)
		{
			DL_LOG("getBestSharedRequest idx " << bestSharedRequest->piece.index);
			return bestSharedRequest;
		}

		return nullptr;
	};

	for (auto idx : sortedSelectedPieces)
	{
		auto& info = piecesDlState[idx];
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

				if (!alreadyRequested && !peer->suspicious())
					possibleShareRequests.push_back(idx);
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

uint32_t mtt::Downloader::sendPieceRequests(PeerCommunication* peer, PeerRequestsInfo::RequestedPiece* request, RequestInfo* r, uint32_t max)
{
	uint32_t count = 0;
	auto blocksCount = (uint16_t)r->piece.blocksState.size();

	uint16_t nextBlock = r->activeRequestsCount % blocksCount;
	for (uint32_t i = 0; i < blocksCount; i++)
	{
		if (r->piece.blocksState.empty() || r->piece.blocksState[nextBlock] == 0)
		{
			if (!request->blocks[nextBlock])
			{
				auto info = torrent.infoFile.info.getPieceBlockInfo(request->idx, nextBlock);
				DL_LOG("Send block request " << info.index << info.begin);
				peer->requestPieceBlock(info);
				request->blocks[nextBlock] = true;
				request->activeBlocksCount++;
				count++;
			}
		}

		nextBlock = (nextBlock + 1) % blocksCount;

		if (count == max)
			break;
	}
	r->activeRequestsCount += (uint16_t)count;

	return count;
}

bool mtt::Downloader::hasWantedPieces(PeerCommunication* peer)
{
	if (peer->info.pieces.pieces.size() != torrent.infoFile.info.pieces.size())
		return false;

	std::lock_guard<std::mutex> guard(sortedSelectedPiecesMutex);

	for (auto idx : sortedSelectedPieces)
	{
		if (piecesDlState[idx].missing && peer->info.pieces.hasPiece(idx))
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

bool mtt::Downloader::PeerRequestsInfo::suspicious() const
{
	return !sharedSuspicion.empty();
}

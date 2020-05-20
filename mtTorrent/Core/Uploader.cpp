#include "Uploader.h"
#include "Torrent.h"
#include "PeerCommunication.h"

mtt::Uploader::Uploader(TorrentPtr t)
{
	globalBw = BandwidthManager::Get().GetChannel("upload");
	torrent = t;
}

void mtt::Uploader::isInterested(PeerCommunication* p)
{
	p->setChoke(false);
}

void mtt::Uploader::pieceRequest(PeerCommunication* p, PieceBlockInfo& info)
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	
	requestBytes(info.length);
	pendingRequests.push_back({p->shared_from_this(), info});
	sendRequests();
}

void mtt::Uploader::assignBandwidth(int amount)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requestingBytes = false;
	availableBytes += amount;

	sendRequests();
}

void mtt::Uploader::requestBytes(uint32_t amount)
{
	if (amount <= availableBytes || requestingBytes)
		return;
	amount -= availableBytes;

	int returned = BandwidthManager::Get().requestBandwidth(shared_from_this(), amount, 1, &globalBw, 1);

	if (returned == 0)
		requestingBytes = true;
	else
		availableBytes += returned;
}

bool mtt::Uploader::isActive()
{
	return !pendingRequests.empty();
}

void mtt::Uploader::sendRequests()
{
	uint32_t wantedBytes = 0;
	std::vector<UploadRequest> delayedRequests;

	for (auto it = pendingRequests.begin(); it != pendingRequests.end(); it++)
	{
		if (it->peer->isEstablished())
		{
			if (availableBytes >= it->block.length)
			{
				auto block = torrent->files.storage.getPieceBlock(it->block);
				it->peer->sendPieceBlock(block);
				uploaded += it->block.length;
				availableBytes -= it->block.length;

				handledRequests.push_back({ it->peer.get(), it->block.length });
			}
			else
			{
				wantedBytes += it->block.length;
				delayedRequests.push_back(std::move(*it));
			}
		}
	}

	pendingRequests = std::move(delayedRequests);

	if (wantedBytes > 0)
		requestBytes(wantedBytes);
}

void mtt::Uploader::refreshRequest()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto it = pendingRequests.begin(); it != pendingRequests.end();)
	{
		if(!it->peer->isEstablished())
			it = pendingRequests.erase(it);
		else
			it++;
	}
}

std::vector<std::pair<mtt::PeerCommunication*, uint32_t>> mtt::Uploader::popHandledRequests()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	return std::move(handledRequests);
}

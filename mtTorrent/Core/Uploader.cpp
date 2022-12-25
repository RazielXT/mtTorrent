#include "Uploader.h"
#include "Torrent.h"
#include "PeerCommunication.h"

mtt::Uploader::Uploader(Torrent& t) : torrent(t)
{
	globalBw = BandwidthManager::Get().GetChannel("upload");
}

void mtt::Uploader::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	pendingRequests.clear();
	requestingBytes = false;
	availableBytes = 0;
}

void mtt::Uploader::isInterested(PeerCommunication* p)
{
	p->setChoke(false);
}

void mtt::Uploader::cancelRequests(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto it = pendingRequests.begin(); it != pendingRequests.end();)
	{
		if (it->peer == p)
			it = pendingRequests.erase(it);
		else
			it++;
	}
}

void mtt::Uploader::pieceRequest(PeerCommunication* p, const PieceBlockInfo& info)
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	
	requestBytes(info.length);
	pendingRequests.push_back({ p, info });

	if (!requestingBytes)
		torrent.service.io.post([this]() { sendRequests(); });
}

void mtt::Uploader::assignBandwidth(int amount)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requestingBytes = false;
	availableBytes += amount;

	if (isActive())
		torrent.service.io.post([this]() { sendRequests(); });
}

void mtt::Uploader::requestBytes(uint32_t amount)
{
	if (amount <= availableBytes || requestingBytes)
		return;
	amount -= availableBytes;

	uint32_t returned = BandwidthManager::Get().requestBandwidth(shared_from_this(), amount, 1, &globalBw, 1);

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
	std::lock_guard<std::mutex> guard(requestsMutex);

	uint32_t wantedBytes = 0;
	size_t delayedCount = 0;

	for (size_t i = 0; i < pendingRequests.size(); i++)
	{
		auto& r = pendingRequests[i];

		if (r.peer->isEstablished())
		{
			if (availableBytes >= r.block.length)
			{
				DataBuffer buffer;
				torrent.files.storage.loadPieceBlock(r.block, buffer);

				PieceBlock block;
				block.info = r.block;
				block.buffer = buffer;

				r.peer->sendPieceBlock(block);
				uploaded += r.block.length;
				availableBytes -= r.block.length;

				handledRequests[r.peer] += r.block.length;
			}
			else
			{
				wantedBytes += r.block.length;

				if (delayedCount < i)
					pendingRequests[delayedCount] = r;

				delayedCount++;
			}
		}
	}

	pendingRequests.resize(delayedCount);

	if (wantedBytes > 0)
		requestBytes(wantedBytes);
}

void mtt::Uploader::refreshRequest()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto it = pendingRequests.begin(); it != pendingRequests.end();)
	{
		if (!it->peer->isEstablished())
			it = pendingRequests.erase(it);
		else
			it++;
	}
}

std::map<mtt::PeerCommunication*, uint32_t> mtt::Uploader::popHandledRequests()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	return std::move(handledRequests);
}

std::string mtt::Uploader::name()
{
	return torrent.name() + "_upload";
}

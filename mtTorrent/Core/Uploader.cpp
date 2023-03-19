#include "Uploader.h"
#include "Torrent.h"
#include "PeerCommunication.h"

#define UP_LOG(x) WRITE_LOG(x)

mtt::Uploader::Uploader(Torrent& t) : torrent(t)
{
	CREATE_NAMED_LOG(Uploader, torrent.name());

	globalBw = BandwidthManager::Get().GetChannel("upload");
}

void mtt::Uploader::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);
	pendingRequests.clear();
	requestingBytes = false;
	availableBytes = 0;
	uploadSpeed = 0;
}

void mtt::Uploader::isInterested(PeerCommunication* p)
{
	UP_LOG("isInterested " << p->getStream()->getAddress());
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
		torrent.service.post([this]() { sendRequests(); });
}

void mtt::Uploader::assignBandwidth(int amount)
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requestingBytes = false;
	availableBytes += amount;
	UP_LOG("assignBandwidth " << amount << ", total " << availableBytes);

	if (isActive())
		torrent.service.post([this]() { sendRequests(); });
}

void mtt::Uploader::requestBytes(uint32_t amount)
{
	UP_LOG("requestBytes " << amount);

	if (amount <= availableBytes || requestingBytes)
		return;
	amount -= availableBytes;

	uint32_t returned = BandwidthManager::Get().requestBandwidth(shared_from_this(), amount, 1, &globalBw, 1);
	UP_LOG("requestBandwidth " << amount << ", returned " << returned);

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
	UP_LOG("sendRequests");
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
				UP_LOG("sendPieceBlock size " << r.block.length << " to " << r.peer->getStream()->getAddress());
				buffer.resize(r.block.length);
				torrent.files.storage.loadPieceBlock(r.block, buffer.data());

				PieceBlock block;
				block.info = r.block;
				block.buffer = buffer;

				r.peer->sendPieceBlock(block);
				uploaded += r.block.length;
				availableBytes -= r.block.length;
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

std::string mtt::Uploader::name()
{
	return torrent.name() + "_upload";
}

#include "DownloadStorage.h"

mtt::DownloadStorage::DownloadStorage(Torrent& t) : torrent(t), cache(pool, t.files.storage)
{
}

void mtt::DownloadStorage::start()
{
	pool.start(2);
}

void mtt::DownloadStorage::stop()
{
	cache.flush();
	pool.stop();

	cache.clear();
	freeCheckBuffers.clear();
}

void mtt::DownloadStorage::checkPiece(uint32_t idx, std::shared_ptr<void> info)
{
	pool.post([this, idx, info]()
		{
			checkPieceFunc(idx, info);
		});
}

void mtt::DownloadStorage::checkPieceFunc(uint32_t idx, std::shared_ptr<void> info)
{
	DataBuffer buffer;
	{
		std::lock_guard<std::mutex> guard(freeCheckBuffersMutex);
		if (!freeCheckBuffers.empty())
		{
			buffer = std::move(freeCheckBuffers.back());
			freeCheckBuffers.pop_back();
		}
	}
	buffer.resize(torrent.getMetadata().info.getPieceSize(idx));

	std::vector<Storage::BlockOffset> missing;
	auto bufferPos = cache.readBlocks(idx, buffer, missing);
	if (bufferPos < buffer.size())
		missing.emplace_back(Storage::BlockOffset{ bufferPos, (uint32_t)(buffer.size() - bufferPos) });

	Status status = missing.empty() ? Status::Success : torrent.files.storage.loadPieceBlocks(idx, missing, buffer.data());
	if (status == Status::Success)
	{
		uint8_t shaBuffer[20] = {};
		_SHA1(buffer.data(), buffer.size(), shaBuffer);
		if (memcmp(shaBuffer, torrent.getMetadata().info.pieces[idx].hash, 20) != 0)
			status = Status::I_Mismatch;
	}
	
	{
		std::lock_guard<std::mutex> guard(freeCheckBuffersMutex);
		freeCheckBuffers.emplace_back(std::move(buffer));
	}

	if (onPieceChecked)
		onPieceChecked(idx, status, std::move(info));
}

void mtt::DownloadStorage::storePieceBlock(const PieceBlock& block)
{
	cache.storeBlock(block);
}

mtt::WriteCache::WriteCache(ServiceThreadpool& p, Storage& s) : pool(p), storage(s)
{
	groupBufferSize = std::max(BlockMaxSize, mtt::config::getInternal().downloadCachePerGroup);
}

void mtt::WriteCache::storeBlock(const PieceBlock& block)
{
	std::lock_guard<std::mutex> guard(cacheMutex);

	GroupIt group;
	uint8_t* buffer = nullptr;
	for (group = cacheGroups.begin(); group != cacheGroups.end(); group++)
	{
		if (buffer = group->getBufferPart(block.buffer.size))
			break;
	}
	if (!buffer)
	{
		group = cacheGroups.emplace(cacheGroups.begin(), Group{ DataBuffer(groupBufferSize) });
		buffer = group->getBufferPart(block.buffer.size);
	}

	memcpy(buffer, block.buffer.data, block.buffer.size);

	group->blocks.emplace_back(Storage::PieceBlockData{ buffer, block.info });

	if (group->full())
		pool.post([this, group]()
			{
				flush(group);
			});
}

uint32_t mtt::WriteCache::readBlocks(uint32_t idx, DataBuffer& buffer, std::vector<Storage::BlockOffset>& missing)
{
	std::vector<Storage::BlockOffset> offsets;
	{
		std::lock_guard<std::mutex> guard(cacheMutex);

		for (const auto& group : cacheGroups)
		{
			for (auto& b : group.blocks)
			{
				if (b.info.index == idx)
				{
					offsets.emplace_back(Storage::BlockOffset{ b.info.begin, b.info.length });
					memcpy(buffer.data() + b.info.begin, b.data, b.info.length);
				}
			}
		}
	}
	
	std::sort(offsets.begin(), offsets.end(), [](const Storage::BlockOffset& l, const Storage::BlockOffset& r){ return l.begin < r.begin; });

	uint32_t pos = 0;
	for (auto& i : offsets)
	{
		if (i.begin > pos)
			missing.emplace_back(Storage::BlockOffset{ pos, i.begin - pos });
		pos = i.begin + i.size;
	}

	return pos;
}

void mtt::WriteCache::flush()
{
	std::lock_guard<std::mutex> guard(cacheMutex);

	for (auto group = cacheGroups.begin(); group != cacheGroups.end(); group++)
	{
		if (!group->full() && !group->blocks.empty())
			pool.post([this, group]()
				{
					flush(group);
				});
	}
}

void mtt::WriteCache::clear()
{
	cacheGroups.clear();
}

void mtt::WriteCache::flush(GroupIt group)
{
	mtt::Status status = Status::Success;
	int retry = 0;
	while ((status = storage.storePieceBlocks(group->blocks)) == mtt::Status::E_FileLockedError && retry < 5)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100 + retry*200));
		retry++;
	}

	{
		std::lock_guard<std::mutex> guard(cacheMutex);
		group->blocks.clear();
		group->bufferPos = 0;
		cacheGroups.splice(cacheGroups.end(), cacheGroups, group); //move back, to prioritize already started group
	}
}

uint8_t* mtt::WriteCache::Group::getBufferPart(size_t size)
{
	if (full())
		return nullptr;

	auto ptr = buffer.data() + bufferPos;
	bufferPos += size;
	return ptr;
}

bool mtt::WriteCache::Group::full() const
{
	return (buffer.size() - bufferPos) < BlockMaxSize;
}

void mtt::UnfinishedPiecesState::add(std::vector<mtt::PieceState>& added)
{
	std::lock_guard<std::mutex> guard(mutex);

	pieces.reserve(pieces.size() + added.size());

	for (auto& p : added)
	{
		pieces.emplace_back(std::move(p));
	}
}

void mtt::UnfinishedPiecesState::clear()
{
	std::lock_guard<std::mutex> guard(mutex);
	pieces.clear();
}

std::vector<mtt::PieceState> mtt::UnfinishedPiecesState::getState()
{
	std::lock_guard<std::mutex> guard(mutex);
	return pieces;
}

uint64_t mtt::UnfinishedPiecesState::getDownloadSize()
{
	uint64_t sz = 0;
	std::lock_guard<std::mutex> guard(mutex);

	for (const auto& u : pieces)
		sz += u.downloadedSize;

	return sz;
}

std::map<uint32_t, uint32_t> mtt::UnfinishedPiecesState::getDownloadSizeMap()
{
	std::map<uint32_t, uint32_t> map;
	std::lock_guard<std::mutex> guard(mutex);

	for (const auto& u : pieces)
		map[u.index] += u.downloadedSize;

	return map;
}

mtt::PieceState mtt::UnfinishedPiecesState::load(uint32_t idx)
{
	std::lock_guard<std::mutex> guard(mutex);

	for (auto& u : pieces)
	{
		if (u.index == idx && u.downloadedSize)
		{
			mtt::PieceState piece = std::move(u);
			u.downloadedSize = 0;

			return piece;
		}
	}
	return {};
}




// void mtt::DownloadStorage::flush(bool all)
// {
// 	std::vector<UnsavedBlocksGroup> groups;
// 	{
// 		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
// 
// 		if (unsavedBlocksGroups.empty())
// 			return;
// 
// 		if (all)
// 			groups = std::move(unsavedBlocksGroups);
// 		else
// 		{
// 			groups.emplace_back(std::move(unsavedBlocksGroups.front()));
// 			unsavedBlocksGroups.erase(unsavedBlocksGroups.begin());
// 		}
// 
// 		flushingCounter++;
// 	}
// 
// 	mtt::Status status = Status::Success;
// 	int retry = 0;
// 	for (auto& group : groups)
// 	{
// 		while ((status = torrent.files.storage.storePieceBlocks(group.blocks)) == mtt::Status::E_FileLockedError && retry < 5)
// 		{
// 			std::this_thread::sleep_for(retry == 0 ? std::chrono::milliseconds(100) : std::chrono::seconds(1));
// 			retry++;
// 		}
// 
// 		{
// 			std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
// 			freeBuffers.emplace_back(std::move(group.buffer));
// 		}
// 	}
// 
// 	{
// 		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
// 		flushingCounter--;
// 	}
// 
// 	flushCv.notify_one();
// 
// 	if (status != Status::Success)
// 	{
// 		torrent.lastError = status;
// 		torrent.stop(Torrent::StopReason::Internal);
// 	}
// }
// 
// void mtt::DownloadStorage::finish()
// {
// 	flush(true);
// 
// 	{
// 		std::unique_lock lk(unsavedPieceBlocksMutex);
// 		flushCv.wait(lk, [&] { return unsavedBlocksGroups.empty() && flushingCounter == 0; });
// 	}
// 
// 	{
// 		std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
// 		freeBuffers.clear();
// 	}
// }
// 
// uint8_t* mtt::DownloadStorage::getBufferPart(size_t size)
// {
// 	if (unsavedBlocksGroups.empty())
// 		return nullptr;
// 
// 	return unsavedBlocksGroups.back().getBufferPart(size);
// }
// 
// 
// uint8_t* mtt::DownloadStorage::getNewBufferPart(size_t size)
// {
// 	if (freeBuffers.empty())
// 		unsavedBlocksGroups.emplace_back(UnsavedBlocksGroup{ DataBuffer(bufferAllocationSize) });
// 	else
// 	{
// 		unsavedBlocksGroups.emplace_back(UnsavedBlocksGroup{ std::move(freeBuffers.back()) });
// 		freeBuffers.pop_back();
// 	}
// 
// 	return getBufferPart(size);
// }
// 
// bool mtt::DownloadStorage::wantsToDownload() const
// {
// 	return !torrent.checking && !torrent.selectionFinished();
// }
// 
// uint8_t* mtt::DownloadStorage::UnsavedBlocksGroup::getBufferPart(size_t size)
// {
// 	if (bufferPos + size <= buffer.size())
// 	{
// 		auto ptr = buffer.data() + bufferPos;
// 		bufferPos += size;
// 		return ptr;
// 	}
// 
// 	return nullptr;
// }
// 
// void mtt::DownloadStorage::BlockCacheGroups::storeBlock(const PieceBlock& block)
// {
// 	std::lock_guard<std::mutex> guard(unsavedPieceBlocksMutex);
// 
// 	auto buffer = getBufferPart(block.buffer.size);
// 
// 	if (!buffer)
// 	{
// 		if (!unsavedBlocksGroups.empty())
// 		{
// 			torrent.service.post([this]()
// 				{
// 					if (wantsToDownload())
// 					flush(false);
// 				});
// 		}
// 
// 		buffer = getNewBufferPart(block.buffer.size);
// 	}
// 	memcpy(buffer, block.buffer.data, block.buffer.size);
// 
// 	unsavedBlocksGroups.back().blocks.emplace_back(Storage::PieceBlockData{ buffer, block.info });
// }

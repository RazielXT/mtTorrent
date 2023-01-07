#pragma once

#include "Torrent.h"
#include "PeerCommunication.h"

namespace mtt
{
	class WriteCache
	{
	public:

		WriteCache(ServiceThreadpool&, Storage&);

		void flush();
		void clear();

		void storeBlock(const PieceBlock& block);
		uint32_t readBlocks(uint32_t idx, DataBuffer&, std::vector<Storage::BlockOffset>& missing);

		size_t groupBufferSize = 1024 * 1024;

	private:

		struct Group
		{
			DataBuffer buffer;
			size_t bufferPos = 0;
			uint8_t* getBufferPart(size_t size);
			bool full() const;

			std::vector<Storage::PieceBlockData> blocks;
		};
		std::list<Group> cacheGroups;
		using GroupIt = std::list<Group>::iterator;

		void flush(GroupIt);

		std::mutex cacheMutex;
		ServiceThreadpool& pool;
		Storage& storage;
	};

	class DownloadStorage
	{
	public:

		DownloadStorage(Torrent&);

		void start();
		void stop();

		void storePieceBlock(const PieceBlock& block);

		void checkPiece(uint32_t idx, std::shared_ptr<void> info);
		std::function<void(uint32_t, Status, std::shared_ptr<void> info)> onPieceChecked;

	private:

		WriteCache cache;

		Torrent& torrent;
		ServiceThreadpool pool;

		void checkPieceFunc(uint32_t idx, std::shared_ptr<void> info);
		std::vector<DataBuffer> freeCheckBuffers;
		std::mutex freeCheckBuffersMutex;
	};

	class UnfinishedPiecesState
	{
	public:

		void add(std::vector<mtt::PieceState>& pieces); //load saved
		mtt::PieceState load(uint32_t idx); //pop unfinished before dl
		std::vector<mtt::PieceState> getState(); //save, pop
		void clear(); //reset

		size_t getDownloadSize();
		std::map<uint32_t, uint32_t> getDownloadSizeMap();

	private:

		std::mutex mutex;
		std::vector<mtt::PieceState> pieces;
	};
}

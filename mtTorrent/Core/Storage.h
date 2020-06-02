#pragma once

#include "Interface.h"
#include <filesystem>
#include <mutex>

namespace mtt
{
	class Storage
	{
	public:

		Storage() {};
		Storage(TorrentInfo& info);
		~Storage();

		void init(TorrentInfo& info, const std::string& locationPath);
		Status setPath(std::string path, bool moveFiles = true);
		std::string getPath();

		Status storePiece(DownloadedPiece& piece);
		PieceBlock getPieceBlock(PieceBlockInfo& piece);

		Status preallocateSelection(DownloadSelection& files);
		DataBuffer checkStoredPieces(std::vector<PieceInfo>& piecesInfo);
		std::shared_ptr<PiecesCheck> checkStoredPiecesAsync(std::vector<PieceInfo>& piecesInfo, asio::io_service& io, std::function<void(std::shared_ptr<PiecesCheck>)> onFinish);
		Status flush();

		Status deleteAll();
		int64_t getLastModifiedTime();

	private:

		void checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo);

		std::filesystem::path getFullpath(File& file);
		void createPath(const std::filesystem::path& path);

		mtt::Status validatePath(DownloadSelection& selection);

		Status flushAllFiles();
		Status flush(File& file);
		Status preallocate(File& file);

		std::string path;

		template<typename T, uint32_t max>
		struct CachedData
		{
			std::array<T, max> data;
			uint32_t nextpos = 0;
			uint32_t count = 0;

			void reset()
			{
				nextpos = count = 0;
			}

			T& getNext()
			{
				count = std::min(max, count + 1);
				T& val = data[nextpos];
				nextpos = (nextpos + 1 == max) ? 0 : nextpos + 1;
				return val;
			}
		};

		CachedData<DownloadedPiece, 6> unsavedPieces;
		std::mutex storageMutex;

		struct CachedPiece
		{
			uint32_t index;
			DataBuffer data;
		};
		CachedData<CachedPiece, 16> cachedPieces;
		std::mutex cacheMutex;

		CachedPiece& loadPiece(uint32_t pieceId);
		void loadPiece(File& file, CachedPiece& piece);

		std::vector<File> files;
		uint32_t pieceSize;
	};
}
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

		Status storePiece(const DownloadedPiece& piece);
		Status loadPieceBlock(const PieceBlockInfo& block, DataBuffer& buffer);

		Status preallocateSelection(const DownloadSelection& files);
		std::vector<uint64_t> getAllocatedSize();
		void checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo, uint32_t workersCount = 1, uint32_t workerIdx = 0);

		Status deleteAll();
		int64_t getLastModifiedTime();

	private:

		std::filesystem::path getFullpath(const File& file);
		void createPath(const std::filesystem::path& path);

		Status validatePath(const DownloadSelection& selection);

		Status preallocate(const File& file);
		Status storePiece(const File& file, const DownloadedPiece& piece);
		Status loadPieceBlock(const File& file, const PieceBlockInfo& block, DataBuffer& buffer);

		std::string path;

		std::mutex storageMutex;

		std::vector<File> files;
		uint32_t pieceSize;
	};
}
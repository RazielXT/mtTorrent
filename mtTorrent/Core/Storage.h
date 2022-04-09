#pragma once

#include "Interface.h"
#include <filesystem>
#include <mutex>

namespace mtt
{
	class Storage
	{
	public:

		Storage(const TorrentInfo& info);
		~Storage();

		void init(const std::string& locationPath);

		Status setPath(std::string path, bool moveFiles = true);
		std::string getPath() const;

		struct PieceBlockRequest
		{
			uint32_t index;
			uint32_t offset;
			const DataBuffer* data;
		};
		Status storePieceBlocks(std::vector<PieceBlockRequest> blocks);

		Status loadPieceBlock(const PieceBlockInfo& block, DataBuffer& buffer);

		Status preallocateSelection(const DownloadSelection& files);
		std::vector<uint64_t> getAllocatedSize() const;
		void checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo, uint32_t workersCount, uint32_t workerIdx, const std::vector<bool>& wantedChecks);

		Status deleteAll();
		uint64_t getLastModifiedTime();
		uint64_t getLastModifiedTime(size_t fileIdx);

	private:

		std::filesystem::path getFullpath(const File& file) const;
		void createPath(const std::filesystem::path& path);

		Status validatePath(const DownloadSelection& selection);

		Status preallocate(const File& file, uint64_t size);
		Status storePieceBlocks(const File& file, const std::vector<PieceBlockRequest>& blocks);
		Status loadPieceBlock(const File& file, const PieceBlockInfo& block, DataBuffer& buffer);

		struct FileBlockPosition
		{
			uint32_t dataPos = 0;
			uint32_t dataSize = {};
			size_t fileDataPos = 0;
		};
		FileBlockPosition getFileBlockPosition(const File& file, uint32_t index, uint32_t offset, uint32_t size);

		std::string path;

		std::mutex storageMutex;

		const TorrentInfo& info;
	};
}
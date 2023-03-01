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

		struct PieceBlockData
		{
			const uint8_t* data;
			PieceBlockInfo info;
		};
		Status storePieceBlocks(std::vector<PieceBlockData>& blocks);

		struct BlockOffset
		{
			uint32_t begin;
			uint32_t size;
		};
		Status loadPieceBlocks(uint32_t idx, const std::vector<BlockOffset>&, uint8_t* buffer);
		Status loadPieceBlock(const PieceBlockInfo&, uint8_t* buffer);

		Status preallocateFile(uint32_t idx);
		Status preallocateSelection(const DownloadSelection& selection);
		Status preallocateSelection(const SelectedFiles& selection);
		uint64_t lastAllocationTime = 0;

		std::vector<uint64_t> getAllocatedSize() const;
		void checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo, uint32_t workersCount, uint32_t workerIdx, const std::vector<bool>& wantedChecks);

		Status deleteAll();
		uint64_t getLastModifiedTime();
		uint64_t getLastModifiedTime(size_t fileIdx);

		static std::filesystem::path utf8Path(const std::string& p);

		PathValidation validatePath(const DownloadSelection& selection) const;
		PathValidation validatePath(const DownloadSelection& selection, const std::string&) const;
		PathValidation validatePath(const SelectedFiles& selection, const std::string&) const;

	private:

		std::filesystem::path getRootpath(const std::string& path) const;
		std::filesystem::path getCurrentRootpath() const;

		std::filesystem::path getFullpath(const File& file, const std::string& path) const;
		std::filesystem::path getFullpath(const File& file) const;
		bool createPath(const std::filesystem::path& path);

		Status preallocate(const File& file, uint64_t size);
		Status storePieceBlocks(const File& file, const std::vector<PieceBlockData>& blocks);
		Status loadPieceBlocks(const File& file, uint32_t idx, const std::vector<BlockOffset>& offsets, uint8_t* buffer);

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
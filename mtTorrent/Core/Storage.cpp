#include "Storage.h"
#include "PiecesProgress.h"
#include "Configuration.h"
#include "utils/SHA.h"
#include "utils/Filesystem.h"
#include <fstream>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

std::filesystem::path mtt::Storage::utf8Path(const std::string& p)
{
#if __cpp_char8_t
	return std::filesystem::path(reinterpret_cast<const char8_t*>(p.c_str()));
#else
	return std::filesystem::u8path(p);
#endif
}

mtt::Storage::Storage(const TorrentInfo& i) : info(i)
{
	init(config::getExternal().files.defaultDirectory);
}

mtt::Storage::~Storage()
{
}

void mtt::Storage::init(const std::string& locationPath)
{
	path = locationPath;
	std::replace(path.begin(), path.end(), '\\', pathSeparator);

	if (!path.empty() && path.back() != pathSeparator)
		path += pathSeparator;
}

mtt::Status mtt::Storage::setPath(std::string p, bool moveFiles)
{
	if (!p.empty() && p.back() != pathSeparator)
		p += pathSeparator;

	if (path != p)
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		std::error_code ec;
		bool pathExists = std::filesystem::exists(utf8Path(p), ec);
		if (ec)
			return mtt::Status::E_InvalidPath;

		if (pathExists && moveFiles && !info.files.empty())
		{
			auto newPath = getRootpath(p);

			if (info.files.size() == 1)
			{
				if (std::filesystem::file_size(newPath, ec))
					return mtt::Status::E_NotEmpty;
			}
			else
			{
				if (std::filesystem::exists(newPath, ec) && !std::filesystem::is_empty(newPath, ec))
					return mtt::Status::E_NotEmpty;
			}

			auto originalPath = getCurrentRootpath();

			if (std::filesystem::exists(originalPath, ec))
			{
				std::filesystem::rename(originalPath, newPath, ec);

				if (ec)
					return mtt::Status::E_AllocationProblem;
			}
		}

		if (!pathExists)
		{
			if (!createPath(utf8Path(p)))
				return mtt::Status::E_InvalidPath;
		}

		path = p;
	}

	return mtt::Status::Success;
}

std::string mtt::Storage::getPath() const
{
	return path;
}

static bool FileContainsPiece(const mtt::File& f, uint32_t pieceIdx)
{
	return f.startPieceIndex <= pieceIdx && f.endPieceIndex >= pieceIdx;
}

mtt::Status mtt::Storage::storePieceBlocks(std::vector<PieceBlockData>& blocks)
{
	std::sort(blocks.begin(), blocks.end(),
		[](const PieceBlockData& l, const PieceBlockData& r) {return l.info.index < r.info.index || (l.info.index == r.info.index && l.info.begin < r.info.begin); });

	uint32_t startIndex = blocks.front().info.index;
	uint32_t lastIndex = blocks.back().info.index;

	auto fIt = std::lower_bound(info.files.begin(), info.files.end(), startIndex, [](const File& f, uint32_t index) { return f.endPieceIndex < index; });

	Status status = Status::E_InvalidInput;
	for (; fIt != info.files.end(); fIt++)
	{
		if (fIt->startPieceIndex > lastIndex)
			break;

		status = storePieceBlocks(*fIt, blocks);

		if (status != Status::Success)
			break;
	}

	return status;
}

mtt::Status mtt::Storage::loadPieceBlock(const PieceBlockInfo& blockInfo, uint8_t* buffer)
{
	return loadPieceBlocks(blockInfo.index, { {blockInfo.begin, blockInfo.length} }, buffer - blockInfo.begin);
}

mtt::Status mtt::Storage::loadPieceBlocks(uint32_t idx, const std::vector<BlockOffset>& offsets, uint8_t* buffer)
{
	Status status = Status::Success;
	auto it = std::lower_bound(info.files.begin(), info.files.end(), idx, [](const File& f, uint32_t index) { return f.endPieceIndex < index; });

	for (; it != info.files.end(); it++)
	{
		if (status != Status::Success || !FileContainsPiece(*it, idx))
			break;

		status = loadPieceBlocks(*it, idx, offsets, buffer);
	}

	return status; 
}

mtt::Status mtt::Storage::loadPieceBlocks(const File& file, uint32_t idx, const std::vector<BlockOffset>& offsets, uint8_t* buffer)
{
	auto path = getFullpath(file);
	std::ifstream fileOut(path, std::ios_base::binary | std::ios_base::in);
	if (!fileOut)
		return Status::E_FileReadError;

	for (auto& offset : offsets)
	{
		auto blockInfo = getFileBlockPosition(file, idx, offset.begin, offset.size);

		if (blockInfo.dataSize > 0)
		{
			if (idx == file.endPieceIndex)
			{
				fileOut.seekg(0, std::ios_base::end);
				auto existingSize = fileOut.tellg();

				if (existingSize < (std::streampos)file.size)
					blockInfo.fileDataPos = info.pieceSize - file.startPiecePos + offset.begin;
			}

			fileOut.seekg(blockInfo.fileDataPos);
			fileOut.read((char*)buffer + offset.begin + blockInfo.dataPos, blockInfo.dataSize);
		}
		else
			break;
	}

	return Status::Success;
}

mtt::Status mtt::Storage::preallocateFile(uint32_t idx)
{
	return preallocateSelection(SelectedFiles(info, idx));
}

mtt::Status mtt::Storage::preallocateSelection(const DownloadSelection& selection)
{
	return preallocateSelection(SelectedFiles(info, selection));
}

mtt::Status mtt::Storage::preallocateSelection(const SelectedFiles& selected)
{
	std::lock_guard<std::mutex> guard(storageMutex);

	auto s = validatePath(selected, path).status;
	if (s != Status::Success)
		return s;

	for (auto file : selected.files)
	{
		auto s = preallocate(info.files[file.idx], file.size);
		if (s != Status::Success)
			return s;
	}

	return Status::Success;
}

std::vector<uint64_t> mtt::Storage::getAllocatedSize() const
{
	std::vector<uint64_t> sizes;
	sizes.reserve(info.files.size());

	for (const auto& f : info.files)
	{
		auto path = getFullpath(f);
		std::error_code ec;

		if (std::filesystem::exists(path, ec))
			sizes.push_back(std::filesystem::file_size(path, ec));
		else
			sizes.push_back(0);
	}

	return sizes;
}

mtt::Status mtt::Storage::deleteAll()
{
	std::lock_guard<std::mutex> guard(storageMutex);

	std::error_code ec;
	for (const auto& f : info.files)
	{
		std::filesystem::remove(getFullpath(f), ec);
	}

	if (info.files.size() > 1)
		std::filesystem::remove_all(getCurrentRootpath(), ec);

	return Status::Success;
}

static uint64_t getWriteTime(const std::filesystem::path& filepath)
{
#ifdef __GNUC__
	struct stat attrib = {};
	stat(filepath.string().data(), &attrib);
	return (uint64_t) attrib.st_mtime;
#else
	std::error_code ec;
	auto tm = std::filesystem::last_write_time(filepath, ec);
	return ec ? 0 : (uint64_t)tm.time_since_epoch().count();
#endif
}

uint64_t mtt::Storage::getLastModifiedTime()
{
	uint64_t time = 0;

	std::lock_guard<std::mutex> guard(storageMutex);

	for (const auto& f : info.files)
	{
		auto path = getFullpath(f);
		auto fileTime = getWriteTime(path);

		if (fileTime > time)
			time = fileTime;
	}

	return time;
}

uint64_t mtt::Storage::getLastModifiedTime(size_t fileIdx)
{
	auto path = getFullpath(info.files[fileIdx]);
	return getWriteTime(path);
}

mtt::Storage::FileBlockPosition mtt::Storage::getFileBlockPosition(const File& file, uint32_t index, uint32_t offset, uint32_t size)
{
	FileBlockPosition blockInfo;
	blockInfo.dataSize = size;

	const auto blockEndPos = offset + blockInfo.dataSize;

	if (file.startPieceIndex == index)
	{
		if (file.startPiecePos > blockEndPos)
			return {};

		if (file.startPiecePos > offset)
		{
			blockInfo.dataPos = file.startPiecePos - offset;
			blockInfo.dataSize -= blockInfo.dataPos;
		}
		else
			blockInfo.fileDataPos = offset - file.startPiecePos;
	}
	else
	{
		blockInfo.fileDataPos = info.pieceSize - file.startPiecePos + offset;

		if (index > file.startPieceIndex + 1)
			blockInfo.fileDataPos += (index - file.startPieceIndex - 1) * (size_t)info.pieceSize;
	}

	if (file.endPieceIndex == index)
	{
		if (file.endPiecePos < offset)
			return {};

		if (file.endPiecePos < blockEndPos)
		{
			blockInfo.dataSize -= blockEndPos - file.endPiecePos;
		}
	}

	return blockInfo;
}

mtt::Status mtt::Storage::storePieceBlocks(const File& file, const std::vector<PieceBlockData>& blocks)
{
	struct FileBlockInfo
	{
		size_t blockIdx;
		FileBlockPosition info;
	};

	std::vector<FileBlockInfo> blocksInfo;
	for (size_t i = 0; i < blocks.size(); i++)
	{
		auto& block = blocks[i];

		if (!FileContainsPiece(file, block.info.index))
			continue;

		auto info = getFileBlockPosition(file, block.info.index, block.info.begin, block.info.length);

		if (info.dataSize == 0)
			continue;

		blocksInfo.push_back({ i, info });
	}

	if (blocksInfo.empty())
		return Status::Success;

	auto path = getFullpath(file);

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

	if (!fileOut)
	{
#ifdef _WIN32
		if (GetLastError() == ERROR_SHARING_VIOLATION)
			return Status::E_FileLockedError;
#endif // _WIN32

		return Status::E_FileWriteError;
	}

	fileOut.seekp(0, std::ios_base::end);
	auto existingSize = fileOut.tellp();

	for (const auto& b : blocksInfo)
	{
		auto& block = blocks[b.blockIdx];

		if (existingSize == file.size || block.info.index == file.startPieceIndex)
		{
			fileOut.seekp(b.info.fileDataPos);
			fileOut.write((const char*)block.data + b.info.dataPos, b.info.dataSize);
		}
		else if (block.info.index == file.endPieceIndex)
		{
			fileOut.seekp(info.pieceSize - file.startPiecePos + block.info.begin);
			fileOut.write((const char*)block.data, b.info.dataSize);
		}
	}

	return Status::Success;
}

void mtt::Storage::checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo, uint32_t workersCount, uint32_t workerIdx, const std::vector<bool>& wantedChecks)
{
	uint32_t currentPieceIdx = 0;

	if (info.files.empty())
		return;

	auto currentFile = &info.files.front();
	auto lastFile = &info.files.back();
	
	std::ifstream fileIn;
	DataBuffer readBuffer(info.pieceSize);
	uint8_t shaBuffer[20] = {};

	while (currentPieceIdx < piecesInfo.size())
	{
		auto fullpath = getFullpath(*currentFile);
		std::error_code ec;
		if ((wantedChecks[currentFile->startPieceIndex] || wantedChecks[currentFile->endPieceIndex]) && std::filesystem::exists(fullpath, ec))
		{
			fileIn.open(fullpath, std::ios_base::binary);

			if (fileIn)
			{
				uint64_t existingSize = std::filesystem::file_size(fullpath, ec);

				if (existingSize == currentFile->size)
				{
					auto readSize = info.pieceSize - currentFile->startPiecePos;
					auto readBufferPos = currentFile->startPiecePos;

					if (currentPieceIdx % workersCount == workerIdx && wantedChecks[currentPieceIdx])
						fileIn.read((char*)readBuffer.data() + readBufferPos, readSize);
					else
						fileIn.seekg(readSize, std::ios_base::beg);

					//all pieces in files with correct size
					while (fileIn && (uint64_t)fileIn.tellg() <= currentFile->size && !checkState.rejected && currentPieceIdx <= currentFile->endPieceIndex)
					{
						if (currentPieceIdx % workersCount == workerIdx && wantedChecks[currentPieceIdx])
						{
							_SHA1(readBuffer.data(), info.pieceSize, shaBuffer);

							if (memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0)
								checkState.progress.addPiece(currentPieceIdx);
						}
						
						checkState.piecesChecked = std::max(checkState.piecesChecked, ++currentPieceIdx);

						if (currentPieceIdx % workersCount == workerIdx && wantedChecks[currentPieceIdx])
							fileIn.read((char*)readBuffer.data(), info.pieceSize);
						else
							fileIn.seekg(info.pieceSize, std::ios_base::cur);
					}

					//end of last file
					if (currentPieceIdx == currentFile->endPieceIndex && currentFile == lastFile)
					{
						if (currentPieceIdx % workersCount == workerIdx && wantedChecks[currentPieceIdx])
						{
							_SHA1(readBuffer.data(), currentFile->endPiecePos, shaBuffer);

							if (memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0)
								checkState.progress.addPiece(currentPieceIdx);
						}

						currentPieceIdx++;
					}
				}
				else
				{
					auto startPieceSize = info.pieceSize - currentFile->startPiecePos;
					auto endPieceSize = currentFile->endPiecePos;
					auto tempFileSize = startPieceSize + endPieceSize;

					//temporary file (not selected but sharing piece data)
					if (existingSize >= startPieceSize && existingSize <= tempFileSize)
					{
						//start piece ending
						if (currentFile->startPieceIndex % workersCount == workerIdx && wantedChecks[currentFile->startPieceIndex])
						{
							fileIn.read((char*)readBuffer.data() + currentFile->startPiecePos, startPieceSize);

							_SHA1(readBuffer.data(), info.pieceSize, shaBuffer);
							if (memcmp(shaBuffer, piecesInfo[currentFile->startPieceIndex].hash, 20) == 0)
								checkState.progress.addPiece(currentFile->startPieceIndex);
						}
						else
							fileIn.seekg(startPieceSize, std::ios_base::cur);

						//end piece starting
						if (existingSize == tempFileSize)
							if (currentFile->endPieceIndex % workersCount == workerIdx && wantedChecks[currentFile->endPieceIndex])
								fileIn.read((char*)readBuffer.data(), endPieceSize);
					}
				}
			}

			fileIn.close();
		}

		if (currentPieceIdx < currentFile->endPieceIndex)
			currentPieceIdx = currentFile->endPieceIndex;

		if (currentFile == lastFile)
			currentPieceIdx = currentFile->endPieceIndex + 1;

		checkState.piecesChecked = std::max(checkState.piecesChecked, currentPieceIdx);

		if (checkState.rejected)
			return;

		if (++currentFile > lastFile)
			break;

		currentPieceIdx = currentFile->startPieceIndex;
	}
}

mtt::Status mtt::Storage::preallocate(const File& file, uint64_t size)
{
	auto fullpath = getFullpath(file);
	if (!createPath(fullpath))
		return Status::E_InvalidPath;

	std::error_code ec;
	bool exists = std::filesystem::exists(fullpath, ec);
	auto existingSize = exists ? std::filesystem::file_size(fullpath, ec) : 0;

	if (ec)
		return Status::E_InvalidPath;

	if (!exists || existingSize != size)
	{
		if (existingSize > size)
		{
			if (size == file.size)
			{
				std::filesystem::resize_file(fullpath, file.size, ec);
				return ec ? Status::E_FileWriteError : Status::Success;
			}

			return Status::Success;
		}

		auto spaceInfo = std::filesystem::space(utf8Path(path), ec);
		if (ec)
			return Status::E_InvalidPath;

		if (spaceInfo.available < size)
			return Status::E_NotEnoughSpace;

		std::fstream fileOut(fullpath, std::ios_base::openmode(std::ios_base::binary | std::ios_base::out | (exists ? std::ios_base::in : 0)));
		if (!fileOut.good())
			return Status::E_InvalidPath;

		auto startPieceSize = info.pieceSize - file.startPiecePos;
		auto endPieceSize = file.endPiecePos;
		auto tempFileSize = startPieceSize + file.endPiecePos;

		//move temporary file end piece
		if (exists && existingSize == tempFileSize && endPieceSize)
		{
			std::vector<char> buffer(endPieceSize);
			fileOut.seekp(startPieceSize);
			fileOut.read(buffer.data(), endPieceSize);
			fileOut.seekp(file.size - 1);
			fileOut.write(buffer.data(), buffer.size());
		}
		else if (size)
		{
			fileOut.seekp(size - 1);
			fileOut.put(0);
		}

		if (fileOut.fail())
			return Status::E_AllocationProblem;

		lastAllocationTime = getWriteTime(fullpath);
	}

	return Status::Success;
}

std::filesystem::path mtt::Storage::getRootpath(const std::string& p) const
{
	return utf8Path(p + info.name);
}

std::filesystem::path mtt::Storage::getCurrentRootpath() const
{
	return getRootpath(path);
}

std::filesystem::path mtt::Storage::getFullpath(const File& file, const std::string& path) const
{
	std::string filePath;
	filePath.reserve(file.name.size() + path.size() + file.path.size() + std::accumulate(file.path.begin(), file.path.end(), (size_t)0, [](size_t sum, const auto& p) { return sum + p.size(); }));
	filePath = path;
	if (!filePath.empty() && filePath.back() != pathSeparator)
		filePath += pathSeparator;

	for (auto& p : file.path)
	{
		filePath += p;
		filePath += pathSeparator;
	}

	filePath += file.name;

	return utf8Path(filePath);
}

std::filesystem::path mtt::Storage::getFullpath(const File& file) const
{
	return getFullpath(file, path);
}

bool mtt::Storage::createPath(const std::filesystem::path& path)
{
	auto folder = path.parent_path();

	if (!std::filesystem::exists(folder))
	{
		std::error_code ec;
		if (!std::filesystem::create_directories(folder, ec))
			return false;
	}

	return true;
}

mtt::PathValidation mtt::Storage::validatePath(const SelectedFiles& selection, const std::string& validatedPath) const
{
	auto dlPath = mtt::Storage::utf8Path(validatedPath);
	std::error_code ec;

	if (dlPath.is_relative())
		dlPath = std::filesystem::absolute(path, ec);

	dlPath = dlPath.root_path();

	if (dlPath.empty() || !std::filesystem::exists(dlPath, ec))
		return { Status::E_InvalidPath };

	PathValidation validation;

	for (auto& selectedFile : selection.files)
	{
		auto fullpath = getFullpath(info.files[selectedFile.idx], validatedPath);
		bool exists = std::filesystem::exists(fullpath, ec);
		auto existingSize = exists ? std::filesystem::file_size(fullpath, ec) : 0;

		if (existingSize < selectedFile.size)
			validation.missingSize += selectedFile.size - existingSize;

		validation.requiredSize += selectedFile.size;
	}

	auto spaceInfo = std::filesystem::space(dlPath, ec);
	if (!ec)
	{
		validation.fullSpace = spaceInfo.capacity;
		validation.availableSpace = spaceInfo.available;

		if (spaceInfo.available < validation.missingSize)
			validation.status = Status::E_NotEnoughSpace;
		else
			validation.status = Status::Success;
	}

	return validation;
}

mtt::PathValidation mtt::Storage::validatePath(const DownloadSelection& selection, const std::string& validatedPath) const
{
	return validatePath(SelectedFiles(info, selection), validatedPath);
}

mtt::PathValidation mtt::Storage::validatePath(const DownloadSelection& selection) const
{
	return validatePath(selection, path);
}

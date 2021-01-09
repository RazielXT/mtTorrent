#include "Storage.h"
#include "utils/ServiceThreadpool.h"
#include "Configuration.h"
#include "utils/SHA.h"
#include "utils/Filesystem.h"
#include <fstream>
#include <iostream>

mtt::Storage::Storage(TorrentInfo& info)
{
	init(info, std::string(".") + pathSeparator);
}

mtt::Storage::~Storage()
{
}

void mtt::Storage::init(TorrentInfo& info, const std::string& locationPath)
{
	pieceSize = info.pieceSize;
	files = info.files;
	path = locationPath;

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
		if (!std::filesystem::exists(std::filesystem::u8path(p), ec))
			return mtt::Status::E_InvalidPath;

		if (moveFiles && !files.empty())
		{
			if (files.size() == 1)
			{
				if (std::filesystem::file_size(std::filesystem::u8path(p), ec))
					return mtt::Status::E_NotEmpty;
			}
			else
			{
				if (!std::filesystem::is_empty(std::filesystem::u8path(p), ec))
					return mtt::Status::E_NotEmpty;
			}

			auto originalPath = path + files.back().path.front();
			if (std::filesystem::exists(std::filesystem::u8path(originalPath), ec))
			{
				auto newPath = p + files.back().path.front();

				for (auto& f : files)
				{
					auto originalPathF = originalPath;
					auto newPathF = newPath;

					for (size_t i = 1; i < f.path.size(); i++)
					{
						if (i + 1 == f.path.size())
						{
							if (!std::filesystem::create_directories(std::filesystem::u8path(newPathF), ec) && ec.value() != 0)
								return mtt::Status::E_AllocationProblem;
						}

						originalPathF += pathSeparator + f.path[i];
						newPathF += pathSeparator + f.path[i];
					}

					if (!std::filesystem::exists(std::filesystem::u8path(originalPathF), ec))
						continue;

					std::filesystem::rename(std::filesystem::u8path(originalPathF), std::filesystem::u8path(newPathF), ec);

					if (ec)
						return mtt::Status::E_AllocationProblem;
				}

				if (files.size() > 1)
					std::filesystem::remove(std::filesystem::u8path(originalPath), ec);
			}
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

mtt::Status mtt::Storage::storePieceBlocks(std::vector<PieceBlockRequest> blocks)
{
	std::sort(blocks.begin(), blocks.end(),
		[](const PieceBlockRequest& l, const PieceBlockRequest& r) {return l.index < r.index || (l.index == r.index && l.offset < r.offset); });

	uint32_t startIndex = blocks.front().index;
	uint32_t lastIndex = blocks.back().index;

	auto fIt = std::lower_bound(files.begin(), files.end(), startIndex, [](const File& f, uint32_t index) { return f.endPieceIndex < index; });

	Status status = Status::E_InvalidInput;
	for (; fIt != files.end(); fIt++)
	{
		if (fIt->startPieceIndex > lastIndex)
			break;

		status = storePieceBlocks(*fIt, blocks);

		if (status != Status::Success)
			break;
	}

	return status;
}

mtt::Status mtt::Storage::storePieceBlock(uint32_t index, uint32_t offset, const DataBuffer& buffer)
{
	std::vector<PieceBlockRequest> blockVec;
	blockVec.push_back({ index, offset, &buffer });

	return storePieceBlocks(std::move(blockVec));
}

mtt::Status mtt::Storage::loadPieceBlock(const PieceBlockInfo& block, DataBuffer& buffer)
{
	Status status = Status::Success;
	buffer.resize(block.length);
	auto it = std::lower_bound(files.begin(), files.end(), block.index, [](const File& f, uint32_t index) { return f.endPieceIndex < index; });

	for (; it != files.end(); it++)
	{
		if (status != Status::Success || !FileContainsPiece(*it, block.index))
			break;

		status = loadPieceBlock(*it, block, buffer);
	}

	return status; 
}

mtt::Status mtt::Storage::preallocateSelection(const DownloadSelection& selection)
{
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		auto s = validatePath(selection);
		if (s != Status::Success)
			return s;

		struct FileAllocations
		{
			const mtt::File& file;
			uint64_t size;
		};
		std::vector<FileAllocations> allocations;

		for (auto it = selection.files.begin(); it != selection.files.end(); it++)
		{
			uint64_t sz = 0;
			if (it->selected)
				sz = it->info.size;
			else
			{
				if (it != selection.files.begin() && (it - 1)->selected)
					sz = pieceSize - it->info.startPiecePos;
				if ((it + 1) != selection.files.end() && (it + 1)->selected)
					sz += it->info.endPiecePos;
			}

			if (sz)
				allocations.push_back({ it->info, sz });
		}

		for (auto a : allocations)
		{
			auto s = preallocate(a.file, a.size);
			if (s != Status::Success)
				return s;
		}
	}

	return Status::Success;
}

std::vector<uint64_t> mtt::Storage::getAllocatedSize() const
{
	std::vector<uint64_t> sizes;
	sizes.reserve(files.size());

	for (auto& f : files)
	{
		auto path = getFullpath(f);
		std::error_code ec;

		if (std::filesystem::exists(path, ec))
			sizes.push_back(std::filesystem::file_size(path, ec));
		else
			sizes.push_back(0);
	}

	return std::move(sizes);
}

mtt::Status mtt::Storage::deleteAll()
{
	std::lock_guard<std::mutex> guard(storageMutex);

	std::error_code ec;
	for (auto& f : files)
	{
		std::filesystem::remove(getFullpath(f), ec);
	}

	if (files.size() > 1)
		std::filesystem::remove_all(std::filesystem::u8path(path + files.front().path.front()), ec);

	return Status::Success;
}

static int64_t getWriteTime(const std::filesystem::path& filepath)
{
#ifdef __GNUC__
	struct stat attrib = {};
	stat(filepath.u8string().data(), &attrib);
	return attrib.st_mtime;
#else
	std::error_code ec;
	auto tm = std::filesystem::last_write_time(filepath, ec);
	return ec ? 0 : tm.time_since_epoch().count();
#endif
}

int64_t mtt::Storage::getLastModifiedTime()
{
	int64_t time = 0;

	std::lock_guard<std::mutex> guard(storageMutex);

	for (auto& f : files)
	{
		auto path = getFullpath(f);
		auto fileTime = getWriteTime(path);

		if (fileTime)
		{
			time = std::max(time, fileTime);
		}
	}

	return time;
}

mtt::Storage::FileBlockPosition mtt::Storage::getFileBlockPosition(const File& file, uint32_t index, uint32_t offset, uint32_t size)
{
	FileBlockPosition info;
	info.dataSize = size;

	const auto blockEndPos = offset + info.dataSize;

	if (file.startPieceIndex == index)
	{
		if (file.startPiecePos > blockEndPos)
			return {};

		if (file.startPiecePos > offset)
		{
			info.dataPos = file.startPiecePos - offset;
			info.dataSize -= info.dataPos;
		}
		else
			info.fileDataPos = offset - file.startPiecePos;
	}
	else
	{
		info.fileDataPos = pieceSize - file.startPiecePos + offset;

		if (index > file.startPieceIndex + 1)
			info.fileDataPos += (index - file.startPieceIndex - 1) * (size_t)pieceSize;
	}

	if (file.endPieceIndex == index)
	{
		if (file.endPiecePos < offset)
			return {};

		if (file.endPiecePos < blockEndPos)
		{
			info.dataSize -= blockEndPos - file.endPiecePos;
		}
	}

	return info;
}

mtt::Status mtt::Storage::storePieceBlocks(const File& file, const std::vector<PieceBlockRequest>& blocks)
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

		if (!FileContainsPiece(file, block.index))
			continue;

		auto info = getFileBlockPosition(file, block.index, block.offset, (uint32_t)block.data->size());

		if (info.dataSize == 0)
			continue;

		blocksInfo.push_back({ i, info });
	}

	if (blocksInfo.empty())
		return Status::Success;

	auto path = getFullpath(file);

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

	if (!fileOut)
		return Status::E_FileWriteError;

	fileOut.seekp(0, std::ios_base::end);
	auto existingSize = fileOut.tellp();

	for (const auto& b : blocksInfo)
	{
		auto& block = blocks[b.blockIdx];

		if (existingSize == file.size || block.index == file.startPieceIndex)
		{
			fileOut.seekp(b.info.fileDataPos);
			fileOut.write((const char*)block.data->data() + b.info.dataPos, b.info.dataSize);
		}
		else if (block.index == file.endPieceIndex)
		{
			fileOut.seekp(pieceSize - file.startPiecePos + block.offset);
			fileOut.write((const char*)block.data->data(), b.info.dataSize);
		}
	}

	return Status::Success;
}

mtt::Status mtt::Storage::loadPieceBlock(const File& file, const PieceBlockInfo& block, DataBuffer& buffer)
{
	auto info = getFileBlockPosition(file, block.index, block.begin, block.length);

	if (info.dataSize > 0)
	{
		auto path = getFullpath(file);
		std::ifstream fileOut(path, std::ios_base::binary | std::ios_base::in);

		if (!fileOut)
			return Status::E_FileReadError;

		if (block.index == file.endPieceIndex)
		{
			fileOut.seekg(0, std::ios_base::end);
			auto existingSize = fileOut.tellg();

			if (existingSize < (std::streampos)file.size)
				info.fileDataPos = pieceSize - file.startPiecePos + block.begin;
		}

		fileOut.seekg(info.fileDataPos);
		fileOut.read((char*)buffer.data() + info.dataPos, info.dataSize);
	}

	return Status::Success;
}

void mtt::Storage::checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo, uint32_t workersCount, uint32_t workerIdx)
{
	checkState.pieces.resize(piecesInfo.size());
	size_t currentPieceIdx = 0;

	if (files.empty())
		return;

	auto currentFile = &files.front();
	auto lastFile = &files.back();
	
	std::ifstream fileIn;
	DataBuffer readBuffer(pieceSize);
	uint8_t shaBuffer[20] = {};

	while (currentPieceIdx < piecesInfo.size())
	{
		auto fullpath = getFullpath(*currentFile);

		std::error_code ec;
		if (std::filesystem::exists(fullpath, ec))
		{
			fileIn.open(fullpath, std::ios_base::binary);

			if (fileIn)
			{
				uint64_t existingSize = std::filesystem::file_size(fullpath, ec);

				if (existingSize == currentFile->size)
				{
					auto readSize = pieceSize - currentFile->startPiecePos;
					auto readBufferPos = currentFile->startPiecePos;

					if (currentPieceIdx % workersCount == workerIdx)
						fileIn.read((char*)readBuffer.data() + readBufferPos, readSize);
					else
						fileIn.seekg(readSize, std::ios_base::beg);

					//all pieces in files with correct size
					while (fileIn && (uint64_t)fileIn.tellg() <= currentFile->size && !checkState.rejected && currentPieceIdx <= currentFile->endPieceIndex)
					{
						if (currentPieceIdx % workersCount == workerIdx)
						{
							_SHA1(readBuffer.data(), pieceSize, shaBuffer);
							checkState.pieces[currentPieceIdx] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
						}
						
						checkState.piecesChecked = std::max(checkState.piecesChecked, (uint32_t)++currentPieceIdx);

						if (currentPieceIdx % workersCount == workerIdx)
							fileIn.read((char*)readBuffer.data(), pieceSize);
						else
							fileIn.seekg(pieceSize, std::ios_base::cur);
					}

					//end of last file
					if (currentPieceIdx == currentFile->endPieceIndex && currentFile == lastFile)
					{
						if (currentPieceIdx % workersCount == workerIdx)
						{
							_SHA1(readBuffer.data(), currentFile->endPiecePos, shaBuffer);
							checkState.pieces[currentPieceIdx] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
						}

						currentPieceIdx++;
					}
				}
				else
				{
					auto startPieceSize = pieceSize - currentFile->startPiecePos;
					auto endPieceSize = currentFile->endPiecePos;
					auto tempFileSize = startPieceSize + endPieceSize;

					//temporary file (not selected but sharing piece data)
					if (existingSize >= startPieceSize && existingSize <= tempFileSize)
					{
						//start piece ending
						if (currentFile->startPieceIndex % workersCount == workerIdx)
						{
							fileIn.read((char*)readBuffer.data() + currentFile->startPiecePos, startPieceSize);

							_SHA1(readBuffer.data(), pieceSize, shaBuffer);
							checkState.pieces[currentFile->startPieceIndex] = memcmp(shaBuffer, piecesInfo[currentFile->startPieceIndex].hash, 20) == 0;
						}
						else
							fileIn.seekg(startPieceSize, std::ios_base::cur);

						//end piece starting
						if(existingSize == tempFileSize)
							if (currentFile->endPieceIndex % workersCount == workerIdx)
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

		checkState.piecesChecked = std::max(checkState.piecesChecked, (uint32_t)currentPieceIdx);

		if (checkState.rejected)
			return;

		if(++currentFile > lastFile)
			break;
	}
}

mtt::Status mtt::Storage::preallocate(const File& file, size_t size)
{
	auto fullpath = getFullpath(file);
	createPath(fullpath);

	std::error_code ec;
	bool exists = std::filesystem::exists(fullpath, ec);
	auto existingSize = exists ? std::filesystem::file_size(fullpath, ec) : 0;

	if (ec)
		return Status::E_InvalidPath;

	if (existingSize != size)
	{
		if (existingSize > size)
		{
			if (size == file.size)
			{
				std::filesystem::resize_file(fullpath, file.size, ec);
				return ec ? Status::E_FileWriteError : Status::Success;
			}
			else
				return Status::Success;
		}

		auto spaceInfo = std::filesystem::space(std::filesystem::u8path(path), ec);
		if (ec)
			return Status::E_InvalidPath;

		if (spaceInfo.available < size)
			return Status::E_NotEnoughSpace;

		std::fstream fileOut(fullpath, std::ios_base::openmode(std::ios_base::binary | std::ios_base::out | (exists ? std::ios_base::in : 0)));
		if(!fileOut.good())
			return Status::E_InvalidPath;

		auto startPieceSize = pieceSize - file.startPiecePos;
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
		else
		{
			fileOut.seekp(size - 1);
			fileOut.put(0);
		}

		if (fileOut.fail())
			return Status::E_AllocationProblem;
	}

	return Status::Success;
}

std::filesystem::path mtt::Storage::getFullpath(const File& file) const
{
	std::string filePath;

	for (auto& p : file.path)
	{
		if (!filePath.empty())
			filePath += pathSeparator;

		filePath += p;
	}

	return std::filesystem::u8path(path + filePath);
}

void mtt::Storage::createPath(const std::filesystem::path& path)
{
	auto folder = path.parent_path();

	if (!std::filesystem::exists(folder))
	{
		std::error_code ec;
		if (!std::filesystem::create_directories(folder, ec))
			return;
	}
}

mtt::Status mtt::Storage::validatePath(const DownloadSelection& selection)
{
	std::filesystem::path dlPath = std::filesystem::u8path(path);
	std::error_code ec;

	if (!dlPath.has_root_path())
	{
		if (dlPath.is_relative())
		{
			if (!std::filesystem::exists(dlPath, ec))
				return Status::E_InvalidPath;
		}
	}
	else
	{
		auto root = dlPath.root_path();

		if (!std::filesystem::exists(root, ec))
			return Status::E_InvalidPath;
	}

	uint64_t fullSize = 0;
	for (auto& f : selection.files)
	{
		if (f.selected)
		{
			auto fullpath = getFullpath(f.info);
			bool exists = std::filesystem::exists(fullpath, ec);
			auto existingSize = exists ? std::filesystem::file_size(fullpath, ec) : 0;

			if (existingSize < f.info.size)
				fullSize += f.info.size - existingSize;
		}
	}

	auto spaceInfo = std::filesystem::space(dlPath, ec);
	if (spaceInfo.available < fullSize)
		return Status::E_NotEnoughSpace;

	return Status::Success;
}


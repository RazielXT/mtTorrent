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

		if (moveFiles && !files.empty())
		{
			auto originalPath = path + files.back().path.front();
			std::error_code ec;
			if (std::filesystem::exists(originalPath, ec))
			{
				if (!std::filesystem::exists(p, ec))
					return mtt::Status::E_InvalidPath;

				auto newPath = p + files.back().path.front();

				for (auto& f : files)
				{
					auto originalPathF = originalPath;
					auto newPathF = newPath;

					for (size_t i = 1; i < f.path.size(); i++)
					{
						if (i + 1 == f.path.size())
						{
							if (!std::filesystem::create_directories(newPathF, ec) && ec.value() != 0)
								return mtt::Status::E_AllocationProblem;
						}

						originalPathF += pathSeparator + f.path[i];
						newPathF += pathSeparator + f.path[i];
					}

					if (!std::filesystem::exists(originalPathF, ec))
						continue;

					std::filesystem::rename(originalPathF, newPathF, ec);

					if (ec)
						return mtt::Status::E_AllocationProblem;
				}

				if (files.size() > 1)
					std::filesystem::remove(originalPath, ec);
			}
		}

		path = p;
	}

	return mtt::Status::Success;
}

std::string mtt::Storage::getPath()
{
	return path;
}

static bool FileContainsPiece(const mtt::File& f, uint32_t pieceIdx)
{
	return f.startPieceIndex <= pieceIdx && f.endPieceIndex >= pieceIdx;
}

mtt::Status mtt::Storage::storePiece(const DownloadedPiece& piece)
{
	std::lock_guard<std::mutex> guard(storageMutex);

	Status status = Status::Success;
	auto it = std::lower_bound(files.begin(), files.end(), piece.index, [](const File& f, uint32_t index) { return f.endPieceIndex < index; });
	
	for (; it != files.end(); it++)
	{
		if (status != Status::Success || !FileContainsPiece(*it, piece.index))
			break;

		status = storePiece(*it, piece);
	}

	return status;
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

mtt::Status mtt::Storage::loadPieceBlock(const File& file, const PieceBlockInfo& block, DataBuffer& buffer)
{
	uint32_t blockDataPos = 0;
	uint32_t dataSize = block.length;
	size_t fileDataPos = {};

	const auto blockEndPos = block.begin + block.length;

	if (file.startPieceIndex == block.index)
	{
		if (file.startPiecePos > blockEndPos)
			return Status::Success;

		if (file.startPiecePos > block.begin)
		{
			blockDataPos = file.startPiecePos - block.begin;
			dataSize -= blockDataPos;
		}
		else
			fileDataPos = block.begin - file.startPiecePos;
	}
	else
	{
		fileDataPos = pieceSize - file.startPiecePos + block.begin;
		if (block.index > file.startPieceIndex + 1)
			fileDataPos += (block.index - file.startPieceIndex - 1) * (size_t)pieceSize;
	}

	if (file.endPieceIndex == block.index)
	{
		if (file.endPiecePos < block.begin)
			return Status::Success;

		if (file.endPiecePos < blockEndPos)
		{
			dataSize -= blockEndPos - file.endPiecePos;
		}
	}

	if (dataSize > 0)
	{
		auto path = getFullpath(file);
		std::ifstream fileOut(path, std::ios_base::binary | std::ios_base::in);

		if (!fileOut)
			return Status::E_FileReadError;

		fileOut.seekg(fileDataPos);
		fileOut.read((char*)buffer.data() + blockDataPos, dataSize);
	}

	return Status::Success;
}

mtt::Status mtt::Storage::preallocateSelection(const DownloadSelection& selection)
{
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		auto s = validatePath(selection);
		if (s != Status::Success)
			return s;

		for (auto& f : selection.files)
		{
			if (f.selected)
			{
				auto s = preallocate(f.info);
				if (s != Status::Success)
					return s;
			}
		}
	}

	return Status::Success;
}

std::vector<uint64_t> mtt::Storage::getAllocatedSize()
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

int64_t mtt::Storage::getLastModifiedTime()
{
	int64_t time = 0;

	std::lock_guard<std::mutex> guard(storageMutex);

	for (auto& f : files)
	{
		auto path = getFullpath(f);
		std::error_code ec;
		auto tm = std::filesystem::last_write_time(path, ec);

		if (!ec)
		{
			time = std::max(time, tm.time_since_epoch().count());
		}
	}

	return time;
}

mtt::Status mtt::Storage::storePiece(const File& file, const DownloadedPiece& piece)
{
	auto path = getFullpath(file);
	createPath(path);

	std::error_code ec;
	bool fileExists = std::filesystem::exists(path, ec);
	if (ec)
		return Status::E_InvalidPath;

	uint64_t existingSize = fileExists ? std::filesystem::file_size(path, ec) : 0;
	if (ec)
		return Status::E_FileReadError;

	if (fileExists && existingSize == file.size)
	{
		std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

		if (!fileOut)
			return Status::E_FileWriteError;

		auto pieceDataPos = file.startPieceIndex == piece.index ? file.startPiecePos : 0;
		uint64_t fileDataPos = file.startPieceIndex == piece.index ? 0 : (pieceSize - file.startPiecePos + (piece.index - file.startPieceIndex - 1) * (uint64_t)pieceSize);
		auto pieceDataSize = std::min(file.size, (uint64_t)piece.data.size() - pieceDataPos);

		if (file.endPieceIndex == piece.index)
			pieceDataSize = std::min(pieceDataSize, (uint64_t)file.endPiecePos);

		fileOut.seekp(fileDataPos);
		fileOut.write((const char*)piece.data.data() + pieceDataPos, pieceDataSize);
	}
	else
	{
		std::ofstream tempFileOut(path, fileExists ? (std::ios_base::binary | std::ios_base::in) : std::ios_base::binary);

		if (!tempFileOut)
			return fileExists ? Status::E_FileWriteError : Status::E_InvalidPath;

		if (piece.index == file.startPieceIndex)
		{
			tempFileOut.write((const char*)piece.data.data() + file.startPiecePos, std::min(file.size, (uint64_t)piece.data.size() - file.startPiecePos));
		}
		else if (piece.index == file.endPieceIndex)
		{
			tempFileOut.seekp(pieceSize - file.startPiecePos);
			tempFileOut.write((const char*)piece.data.data(), file.endPiecePos);
		}
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
	uint8_t shaBuffer[20] = { 0 };

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
						
						checkState.piecesChecked = (uint32_t)++currentPieceIdx;

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
					if (existingSize == tempFileSize || existingSize == startPieceSize)
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

		checkState.piecesChecked = (uint32_t)currentPieceIdx;

		if (checkState.rejected)
			return;

		if(++currentFile > lastFile)
			break;
	}
}

mtt::Status mtt::Storage::preallocate(const File& file)
{
	auto fullpath = getFullpath(file);
	createPath(fullpath);

	std::error_code ec;
	bool exists = std::filesystem::exists(fullpath, ec);
	auto existingSize = exists ? std::filesystem::file_size(fullpath, ec) : 0;

	if (ec)
		return Status::E_InvalidPath;

	if (existingSize != file.size)
	{
		if (existingSize > file.size)
		{
			std::filesystem::resize_file(fullpath, file.size, ec);
			return ec ? Status::E_FileWriteError : Status::Success;
		}

		auto spaceInfo = std::filesystem::space(path, ec);
		if (ec)
			return Status::E_InvalidPath;

		if (spaceInfo.available < file.size)
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
			fileOut.seekp(file.size - 1);
			fileOut.put(0);
		}

		if (fileOut.fail())
			return Status::E_AllocationProblem;
	}

	return Status::Success;
}

std::filesystem::path mtt::Storage::getFullpath(const File& file)
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


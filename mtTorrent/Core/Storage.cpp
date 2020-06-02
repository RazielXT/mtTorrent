#include "Storage.h"
#include <fstream>
#include <iostream>
#include "utils/ServiceThreadpool.h"
#include "utils/SHA.h"

mtt::Storage::Storage(TorrentInfo& info)
{
	init(info, ".//");
}

mtt::Storage::~Storage()
{
	flush();
}

void mtt::Storage::init(TorrentInfo& info, const std::string& locationPath)
{
	pieceSize = info.pieceSize;
	files = info.files;
	path = locationPath;

	if (!path.empty() && path.back() != '\\')
		path += '\\';
}

mtt::Status mtt::Storage::setPath(std::string p, bool moveFiles)
{
	if (!p.empty() && p.back() != '\\')
		p += '\\';

	if (path != p)
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		if (files.size() >= 1)
		{
			std::error_code ec;
			auto originalPath = path + files.back().path.front();
			if (std::filesystem::exists(originalPath, ec) && moveFiles)
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

						originalPathF += '\\' + f.path[i];
						newPathF += '\\' + f.path[i];
					}

					std::filesystem::rename(originalPathF, newPathF, ec);

					if (ec)
						return mtt::Status::E_AllocationProblem;
				}

				if (files.size() > 1)
					std::filesystem::remove(originalPath, ec);
			}

			path = p;
		}
	}

	return Status::Success;
}

std::string mtt::Storage::getPath()
{
	return path;
}

mtt::Status mtt::Storage::storePiece(DownloadedPiece& piece)
{
	std::lock_guard<std::mutex> guard(storageMutex);

	unsavedPieces.getNext() = piece;

	if (unsavedPieces.count == unsavedPieces.data.size())
		return flushAllFiles();

	return Status::Success;
}

mtt::PieceBlock mtt::Storage::getPieceBlock(PieceBlockInfo& block)
{
	PieceBlock out;
	out.info = block;

	std::lock_guard<std::mutex> guard(cacheMutex);

	auto& piece = loadPiece(block.index);

	if (piece.data.size() >= block.begin + block.length)
	{
		out.buffer.store(piece.data.data() + block.begin, block.length);
	}

	return out;
}

mtt::Storage::CachedPiece& mtt::Storage::loadPiece(uint32_t pieceId)
{
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		for (uint32_t i = 0; i < unsavedPieces.count; i++)
		{
			auto& p = unsavedPieces.data[i];

			if (p.index == pieceId)
			{
				auto& c = cachedPieces.getNext();
				c.data = p.data;
				c.index = p.index;
			}
		}
	}

	for (uint32_t i = 0; i < cachedPieces.count; i++)
	{
		if (cachedPieces.data[i].index == pieceId)
			return cachedPieces.data[i];
	}

	auto& piece = cachedPieces.getNext();
	piece.index = pieceId;
	piece.data.resize(pieceSize);

	for (auto& f : files)
	{
		if (f.startPieceIndex <= piece.index && f.endPieceIndex >= piece.index)
		{
			loadPiece(f, piece);
		}
	}

	if(files.back().endPieceIndex == pieceId)
		piece.data.resize(files.back().endPiecePos);

	return piece;
}

void mtt::Storage::loadPiece(File& file, CachedPiece& piece)
{
	size_t fileDataPos = (piece.index == file.startPieceIndex) ? 0 : pieceSize - file.startPiecePos;
	if (piece.index > file.startPieceIndex + 1)
		fileDataPos += (piece.index - file.startPieceIndex - 1)*(size_t)pieceSize;

	uint32_t bufferDataPos = (piece.index == file.startPieceIndex) ? file.startPiecePos : 0;

	auto dataSize = (uint32_t)std::min((uint64_t)pieceSize, file.size);
	if (file.startPieceIndex == piece.index)
		dataSize = std::min(pieceSize - file.startPiecePos, dataSize);
	else if (file.endPieceIndex == piece.index)
		dataSize = std::min(file.endPiecePos, dataSize);

	if (dataSize > 0)
	{
		auto path = getFullpath(file);
		std::ifstream fileOut(path, std::ios_base::binary | std::ios_base::in);

		fileOut.seekg(fileDataPos);
		fileOut.read((char*)piece.data.data() + bufferDataPos, dataSize);
	}
}

mtt::Status mtt::Storage::preallocateSelection(DownloadSelection& selection)
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

	{
		std::lock_guard<std::mutex> guard(cacheMutex);

		cachedPieces.reset();
	}

	return Status::Success;
}

mtt::Status mtt::Storage::flush()
{
	std::lock_guard<std::mutex> guard(storageMutex);

	return flushAllFiles();
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

mtt::Status mtt::Storage::flushAllFiles()
{
	for (auto& f : files)
	{
		auto s = flush(f);
		if (s != Status::Success)
			return s;
	}

	unsavedPieces.reset();
	return Status::Success;
}

mtt::Status mtt::Storage::flush(File& file)
{
	std::vector<DownloadedPiece*> filePieces;
	for (uint32_t i = 0; i < unsavedPieces.count; i++)
	{
		auto& piece = unsavedPieces.data[i];
		if (file.startPieceIndex <= piece.index && file.endPieceIndex >= piece.index)
			filePieces.push_back(&piece);
	}

	if (filePieces.empty())
		return Status::Success;

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

		if (fileOut)
			for (auto& p : filePieces)
			{
				auto pieceDataPos = file.startPieceIndex == p->index ? file.startPiecePos : 0;
				auto fileDataPos = file.startPieceIndex == p->index ? 0 : (pieceSize - file.startPiecePos + (p->index - file.startPieceIndex - 1) * pieceSize);
				auto pieceDataSize = std::min(file.size, (uint64_t)p->data.size() - pieceDataPos);

				if (file.endPieceIndex == p->index)
					pieceDataSize = std::min(pieceDataSize, (uint64_t)file.endPiecePos);

				fileOut.seekp(fileDataPos);
				fileOut.write((const char*)p->data.data() + pieceDataPos, pieceDataSize);
			}
	}
	else
	{
		std::ofstream tempFileOut(path, fileExists ? (std::ios_base::binary | std::ios_base::in) : std::ios_base::binary);

		if (tempFileOut)
			for (auto& p : filePieces)
			{
				if (p->index == file.startPieceIndex)
				{
					tempFileOut.write((const char*)p->data.data() + file.startPiecePos, std::min(file.size, (uint64_t)p->data.size() - file.startPiecePos));
				}
				else if (p->index == file.endPieceIndex)
				{
					tempFileOut.seekp(pieceSize - file.startPiecePos);
					tempFileOut.write((const char*)p->data.data(), file.endPiecePos);
				}

			}
		else
			return fileExists ? Status::E_FileReadError : Status::E_InvalidPath;
	}
	return Status::Success;
}

DataBuffer mtt::Storage::checkStoredPieces(std::vector<PieceInfo>& piecesInfo)
{
	PiecesCheck check;
	checkStoredPieces(check, piecesInfo);
	return check.pieces;
}

void mtt::Storage::checkStoredPieces(PiecesCheck& checkState, const std::vector<PieceInfo>& piecesInfo)
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
				uint64_t existingSize = std::filesystem::file_size(fullpath);

				if (existingSize == currentFile->size)
				{
					auto readSize = pieceSize - currentFile->startPiecePos;
					auto readBufferPos = currentFile->startPiecePos;

					fileIn.read((char*)readBuffer.data() + readBufferPos, readSize);

					while (fileIn && !checkState.rejected && currentPieceIdx <= currentFile->endPieceIndex)
					{
						_SHA1(readBuffer.data(), pieceSize, shaBuffer);
						checkState.pieces[currentPieceIdx] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
						fileIn.read((char*)readBuffer.data(), pieceSize);

						checkState.piecesChecked = (uint32_t)++currentPieceIdx;
					}

					if (currentPieceIdx == currentFile->endPieceIndex && currentFile == lastFile)
					{
						_SHA1(readBuffer.data(), currentFile->endPiecePos, shaBuffer);
						checkState.pieces[currentPieceIdx] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
						++currentPieceIdx;
					}
				}
				else
				{
					auto startPieceSize = pieceSize - currentFile->startPiecePos;
					auto endPieceSize = currentFile->endPiecePos;
					auto tempFileSize = startPieceSize + endPieceSize;

					if (existingSize == tempFileSize || existingSize == startPieceSize)
					{
						fileIn.read((char*)readBuffer.data() + currentFile->startPiecePos, startPieceSize);

						_SHA1(readBuffer.data(), pieceSize, shaBuffer);
						checkState.pieces[currentFile->startPieceIndex] = memcmp(shaBuffer, piecesInfo[currentFile->startPieceIndex].hash, 20) == 0;

						if(existingSize == tempFileSize)
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

std::shared_ptr<mtt::PiecesCheck> mtt::Storage::checkStoredPiecesAsync(std::vector<PieceInfo>& piecesInfo, asio::io_service& io, std::function<void(std::shared_ptr<PiecesCheck>)> onFinish)
{
	auto request = std::make_shared<mtt::PiecesCheck>();
	request->piecesCount = (uint32_t)piecesInfo.size();

	io.post([piecesInfo, onFinish, request, this]()
	{
		checkStoredPieces(*request.get(), piecesInfo);

		onFinish(request);
	});

	return request;
}

mtt::Status mtt::Storage::preallocate(File& file)
{
	auto fullpath = getFullpath(file);
	createPath(fullpath);
	if (!std::filesystem::exists(fullpath) || std::filesystem::file_size(fullpath) != file.size)
	{
		std::error_code ec;
		auto spaceInfo = std::filesystem::space(path, ec);
		if (ec)
			return Status::E_InvalidPath;

		if (spaceInfo.available < file.size)
			return Status::E_NotEnoughSpace;

		std::ofstream fileOut(fullpath, std::ios_base::binary);
		fileOut.seekp(file.size - 1);
		fileOut.put(0);

		if (fileOut.fail())
			return Status::E_AllocationProblem;
	}

	return Status::Success;
}

std::filesystem::path mtt::Storage::getFullpath(File& file)
{
	std::string filePath;

	for (auto& p : file.path)
	{
		if (!filePath.empty())
			filePath += "\\";

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

mtt::Status mtt::Storage::validatePath(DownloadSelection& selection)
{
	std::filesystem::path dlPath = std::filesystem::u8path(path);

	if(!dlPath.has_root_path())
		return Status::E_InvalidPath;

	auto root = dlPath.root_path();
	std::error_code ec;
	if (!std::filesystem::exists(root, ec))
		return Status::E_InvalidPath;

	uint64_t fullsize = 0;
	for (auto& f : selection.files)
	{
		if (f.selected)
		{
			fullsize += f.info.size;
		}
	}

	auto spaceInfo = std::filesystem::space(root, ec);
	if (spaceInfo.available < fullsize)
		return Status::E_NotEnoughSpace;

	return Status::Success;
}


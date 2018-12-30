#include "Storage.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <iostream>
#include "utils/ServiceThreadpool.h"

mtt::Storage::Storage(TorrentInfo& info)
{
	init(info);
	path = ".//";
}

mtt::Storage::~Storage()
{
	flush();
}

void mtt::Storage::init(TorrentInfo& info)
{
	pieceSize = info.pieceSize;
	files = info.files;

	DownloadSelection selection;
	for (auto&f : info.files)
	{
		selection.files.push_back({ false, f });
	}

	preallocateSelection(selection);
}

void mtt::Storage::setPath(std::string p)
{
	path = p;

	if (!path.empty() && path.back() != '\\')
		path += '\\';
}

void mtt::Storage::storePiece(DownloadedPiece& piece)
{
	std::lock_guard<std::mutex> guard(storageMutex);

	unsavedPieces.getNext() = piece;

	if (unsavedPieces.count == unsavedPieces.data.size())
		flushAllFiles();
}

mtt::PieceBlock mtt::Storage::getPieceBlock(PieceBlockInfo& block)
{
	PieceBlock out;
	out.info = block;

	std::lock_guard<std::mutex> guard(cacheMutex);

	auto& piece = loadPiece(block.index);

	if (piece.data.size() >= block.begin + block.length)
	{
		out.data.resize(block.length);
		memcpy(out.data.data(), piece.data.data() + block.begin, block.length);
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

	auto dataSize = (uint32_t)std::min((size_t)pieceSize, file.size);
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

void mtt::Storage::preallocateSelection(DownloadSelection& selection)
{
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		for (auto& f : selection.files)
		{
			if (f.selected)
				preallocate(f.info);
		}
	}

	{
		std::lock_guard<std::mutex> guard(cacheMutex);

		cachedPieces.reset();
	}
}

void mtt::Storage::flush()
{
	std::lock_guard<std::mutex> guard(storageMutex);

	flushAllFiles();
}

void mtt::Storage::flushAllFiles()
{
	for (auto& f : files)
	{
		flush(f);
	}

	unsavedPieces.reset();
}

void mtt::Storage::flush(File& file)
{
	std::vector<DownloadedPiece*> filePieces;
	for (uint32_t i = 0; i < unsavedPieces.count; i++)
	{
		auto& piece = unsavedPieces.data[i];
		if (file.startPieceIndex <= piece.index && file.endPieceIndex >= piece.index)
			filePieces.push_back(&piece);
	}

	if (filePieces.empty())
		return;

	auto path = getFullpath(file);
	createPath(path);

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

	for (auto& p : filePieces)
	{
		auto pieceDataPos = file.startPieceIndex == p->index ? file.startPiecePos : 0;
		auto fileDataPos = file.startPieceIndex == p->index ? 0 : (pieceSize - file.startPiecePos + (p->index - file.startPieceIndex - 1)*pieceSize);
		auto pieceDataSize = std::min(file.size, p->data.size() - pieceDataPos);

		fileOut.seekp(fileDataPos);
		fileOut.write((const char*)p->data.data() + pieceDataPos, pieceDataSize);
	}
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
		boost::filesystem::path dir(fullpath);

		if (boost::filesystem::exists(dir))
		{
			fileIn.open(fullpath, std::ios_base::binary);

			if (fileIn)
			{
				auto readSize = pieceSize - currentFile->startPiecePos;
				auto readBufferPos = currentFile->startPiecePos;

				fileIn.read((char*)readBuffer.data() + readBufferPos, readSize);

				while (fileIn && !checkState.rejected)
				{
					SHA1(readBuffer.data(), pieceSize, shaBuffer);
					checkState.pieces[currentPieceIdx++] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
					fileIn.read((char*)readBuffer.data(), pieceSize);

					checkState.piecesChecked = (uint32_t)currentPieceIdx;
				}

				if (checkState.rejected)
					return;

				if (currentPieceIdx == currentFile->endPieceIndex && currentFile == lastFile)
				{
					SHA1(readBuffer.data(), currentFile->endPiecePos, shaBuffer);
					checkState.pieces[currentPieceIdx++] = memcmp(shaBuffer, piecesInfo[currentPieceIdx].hash, 20) == 0;
				}
			}

			fileIn.close();
		}

		if (currentPieceIdx < currentFile->endPieceIndex)
			currentPieceIdx = currentFile->endPieceIndex;

		if (currentFile == lastFile)
			currentPieceIdx = currentFile->endPieceIndex + 1;

		checkState.piecesChecked = (uint32_t)currentPieceIdx;

		if(++currentFile > lastFile)
			break;
	}
}

std::shared_ptr<mtt::PiecesCheck> mtt::Storage::checkStoredPiecesAsync(std::vector<PieceInfo>& piecesInfo, boost::asio::io_service& io, std::function<void(std::shared_ptr<PiecesCheck>)> onFinish)
{
	auto request = std::make_shared<mtt::PiecesCheck>();
	request->piecesCount = (uint32_t)piecesInfo.size();

	io.post([piecesInfo, onFinish, request, this]()
	{
		checkStoredPieces(*request.get(), piecesInfo);

		if(!request->rejected)
			onFinish(request);
	});

	return request;
}

void mtt::Storage::preallocate(File& file)
{
	auto fullpath = getFullpath(file);
	createPath(fullpath);
	boost::filesystem::path dir(fullpath);
	if (!boost::filesystem::exists(dir) || boost::filesystem::file_size(dir) != file.size)
	{
		std::ofstream fileOut(fullpath, std::ios_base::binary);
		fileOut.seekp(file.size - 1);
		fileOut.put(0);
	}
}

std::string mtt::Storage::getFullpath(File& file)
{
	std::string filePath;

	for (auto& p : file.path)
	{
		if (!filePath.empty())
			filePath += "\\";

		filePath += p;
	}

	return path + filePath;
}

void mtt::Storage::createPath(std::string& path)
{
	auto i = path.find_last_of('\\');

	if (i != std::string::npos)
	{
		std::string dirPath = path.substr(0, i);
		boost::filesystem::path dir(dirPath);
		if (!boost::filesystem::exists(dir))
		{
			if (!boost::filesystem::create_directory(dir))
				return;
		}
	}
}


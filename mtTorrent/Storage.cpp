#include "Storage.h"
#include <fstream>
#include <boost/filesystem.hpp>

mtt::Storage::Storage(uint32_t piecesz)
{
	pieceSize = piecesz;
	path = ".//";
}

void mtt::Storage::setPath(std::string p)
{
	path = p;
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

const uint32_t blockRequestMaxSize = 16 * 1024;

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

	for (auto& s : selection.files)
	{
		auto& f = s.info;

		if (f.startPieceIndex <= piece.index && f.endPieceIndex >= piece.index)
		{
			loadPiece(f, piece);
		}
	}

	if(selection.files.back().info.endPieceIndex == pieceId)
		piece.data.resize(selection.files.back().info.endPiecePos);

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

std::vector<mtt::PieceBlockInfo> mtt::Storage::makePieceBlocksInfo(uint32_t index)
{
	std::vector<PieceBlockInfo> out;
	uint32_t size = pieceSize;

	if (index == selection.files.back().info.endPieceIndex)
		size = selection.files.back().info.endPiecePos;

	for (int i = 0; i*blockRequestMaxSize < size; i++)
	{
		PieceBlockInfo block;
		block.begin = i*blockRequestMaxSize;
		block.index = index;
		block.length = std::min(size - block.begin, blockRequestMaxSize);

		out.push_back(block);
	}

	return out;
}

void mtt::Storage::setSelection(DownloadSelection& newSelection)
{
	{
		std::lock_guard<std::mutex> guard(storageMutex);

		selection = newSelection;

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
	for (auto& s : selection.files)
	{
		flush(s.info);
	}

	unsavedPieces.reset();
}

void mtt::Storage::saveProgress()
{

}

void mtt::Storage::loadProgress()
{

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
	std::string dirPath = path.substr(0, path.find_last_of('\\'));
	boost::filesystem::path dir(dirPath);
	if (!boost::filesystem::exists(dir))
	{
		if (!boost::filesystem::create_directory(dir))
			return;
	}

	std::ofstream fileOut(path, std::ios_base::binary | std::ios_base::in);

	for (auto& p : filePieces)
	{
		auto pieceDataPos = file.startPieceIndex == p->index ? file.startPiecePos : 0;
		auto fileDataPos = file.startPieceIndex == p->index ? 0 : (pieceSize - file.startPiecePos + (p->index - file.startPieceIndex - 1)*pieceSize);
		auto pieceDataSize = std::min(file.size, p->dataSize - pieceDataPos);

		fileOut.seekp(fileDataPos);
		fileOut.write((const char*)p->data.data() + pieceDataPos, pieceDataSize);
	}
}

std::vector<mtt::PieceInfo> mtt::Storage::checkFileHash(File& file)
{
	std::lock_guard<std::mutex> guard(storageMutex);

	std::vector<mtt::PieceInfo> out;

	auto fullpath = getFullpath(file);
	boost::filesystem::path dir(fullpath);
	if (boost::filesystem::exists(dir))
	{
		std::ifstream fileIn(fullpath, std::ios_base::binary);
		std::vector<char> buffer(pieceSize);
		size_t idx = 0;
		
		if (file.startPiecePos)
		{
			auto startOffset = pieceSize - (file.startPiecePos % pieceSize);
			out.push_back(PieceInfo{});
			fileIn.read(buffer.data(), startOffset);
		}	

		PieceInfo info;
		out.reserve(file.size % pieceSize);

		while (fileIn)
		{
			fileIn.read(buffer.data(), pieceSize);
			SHA1((unsigned char*)buffer.data(), fileIn.gcount(), (unsigned char*)info.hash);
			out.push_back(info);
		}
	}

	return out;
}

void mtt::Storage::preallocate(File& file)
{
	auto fullpath = getFullpath(file);
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


#include "Storage.h"
#include <fstream>
#include <boost/filesystem.hpp>

using namespace Torrent;

Storage::Storage(size_t piecesz)
{
	pieceSize = piecesz;
}

void Storage::storePiece(DownloadedPiece& piece)
{
	bool added = false;

	if (piece.index == 791)
		added = true;

	for (auto& s : selection.files)
	{
		auto& f = s.file;

		if (f.startPieceIndex <= piece.index && f.endPieceIndex >= piece.index)
		{
			if (s.storageType == StorageType::Memory)
				memoryStorage.storePiece(f, piece, pieceSize);
		}
	}
}

void Storage::exportFiles(std::string path)
{
	for (auto& s : selection.files)
	{
		std::string filePath;

		for (auto& p : s.file.path)
		{
			if (!filePath.empty())
				filePath += "\\";

			filePath += p;
		}

		filePath = path + filePath;

		if (s.storageType == StorageType::Memory)
			memoryStorage.exportFile(s.file, filePath);
	}
}

void Torrent::Storage::selectFiles(std::vector<Torrent::File>& files)
{
	selection.files.clear();

	for (auto& f : files)
	{
		auto storageType = StorageType::Memory;

		selection.files.push_back({ f, storageType });

		if (storageType == StorageType::Memory)
			memoryStorage.prepare(f, pieceSize);
		if (storageType == StorageType::File)
			fileStorage.prepare(f);
	}
}

void MemoryStorage::storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize)
{
	auto& outBuffer = filesBuffer[file.id];

	size_t bufferStartOffset = (piece.index - file.startPieceIndex)*normalPieceSize;

	memcpy(&outBuffer[0] + bufferStartOffset, piece.data.data(), normalPieceSize);
}

void MemoryStorage::exportFile(File& file, std::string path)
{
	std::string dirPath = path.substr(0, path.find_last_of('\\'));
	boost::filesystem::path dir(dirPath);
	if (!boost::filesystem::exists(dir)) 
	{
		if (!boost::filesystem::create_directory(dir))
			return;
	}

	std::ofstream fileOut(path, std::ios_base::binary);
	auto& data = filesBuffer[file.id];
	fileOut.write(data.data() + file.startPiecePos, file.size);
}

void Torrent::MemoryStorage::prepare(File& file, size_t normalPieceSize)
{
	auto& data = filesBuffer[file.id];
	auto pieces = file.endPieceIndex - file.startPieceIndex + 1;

	if (data.size() != pieces*normalPieceSize)
	{
		data.resize(pieces*normalPieceSize);
	}
}


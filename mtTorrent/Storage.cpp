#include "Storage.h"
#include <fstream>

using namespace Torrent;

Storage::Storage(DownloadSelection& s, size_t piecesz) : selection(s)
{
	pieceSize = piecesz;
}

void Storage::storePiece(DownloadedPiece& piece)
{
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
				filePath += "//";

			filePath += p;
		}

		filePath = path + filePath;

		if (s.storageType == StorageType::Memory)
			memoryStorage.exportFile(s.file, filePath);
	}
}

void Torrent::Storage::selectionChanged()
{
	for (auto& s : selection.files)
	{
		if (s.storageType == StorageType::Memory)
			memoryStorage.prepare(s.file);
		if (s.storageType == StorageType::File)
			fileStorage.prepare(s.file);
	}
}


void MemoryStorage::storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize)
{
	auto& outBuffer = filesBuffer[file.id];

	auto pieceStart = piece.data.data();
	auto dataEnd = pieceStart + piece.dataSize;

	if (piece.index == file.startPieceIndex)
		pieceStart = pieceStart + file.startPiecePos;

	if (piece.index == file.endPieceIndex)
		dataEnd = std::min(dataEnd, pieceStart + file.endPiecePos);

	size_t bufferStartOffset = (piece.index - file.startPieceIndex)*normalPieceSize - file.startPiecePos;

	memcpy(&outBuffer[0] + bufferStartOffset, pieceStart, dataEnd - pieceStart);
}

void MemoryStorage::exportFile(File& file, std::string path)
{
	std::ofstream fileOut(path, std::ios_base::binary);

	auto& data = filesBuffer[file.id];
	fileOut.write(data.data(), data.size());
}

void Torrent::MemoryStorage::prepare(File& file)
{
	auto& data = filesBuffer[file.id];

	if (data.size() != file.size)
	{
		data.resize(file.size);
	}
}


#include "Storage.h"
#include <fstream>

using namespace Torrent;

Storage::Storage(DownloadSelection& s, size_t piecesz) : selection(s)
{
	pieceSize = piecesz;
	selectionChanged();
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
		s.storageType = StorageType::Memory;

		if (s.storageType == StorageType::Memory)
			memoryStorage.prepare(s.file, pieceSize);
		if (s.storageType == StorageType::File)
			fileStorage.prepare(s.file);
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


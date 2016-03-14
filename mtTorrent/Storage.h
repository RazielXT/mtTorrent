#pragma once

#include "Interface.h"

namespace Torrent
{
	class MemoryStorage
	{
	public:

		void prepare(File& file, size_t normalPieceSize);
		void storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize);

		std::map<int, DataBuffer> filesBuffer;
		void exportFile(File& file, std::string path);
	};

	class FileStorage
	{
	public:

		void flush() {};
		void prepare(File& file) {};
		void storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize) {};

		std::vector<DownloadedPiece> cachedPieces;
	};

	class Storage
	{
	public:

		Storage(DownloadSelection& selection, size_t pieceSize);

		void storePiece(DownloadedPiece& piece);
		void exportFiles(std::string path);
		void selectionChanged();

		MemoryStorage memoryStorage;
		FileStorage fileStorage;

		DownloadSelection& selection;
		size_t pieceSize;
	};
}
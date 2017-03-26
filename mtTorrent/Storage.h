#pragma once

#include "TorrentDefines.h"

namespace mtt
{
	class Storage
	{
	public:

		Storage(size_t pieceSize);

		void setPath(std::string path);

		void storePiece(DownloadedPiece& piece);
		void exportFiles();
		void selectFiles(std::vector<mtt::File>& files);

		void saveProgress();
		void loadProgress();

		DownloadSelection selection;
		size_t pieceSize;

	private:

		std::string getFullpath(File& file);
		void storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize);

		const int piecesCacheSize = 5;
		void flush(File& file);

		std::string path;
		std::map<int, std::vector<DownloadedPiece>> unsavedPieces;

	};
}
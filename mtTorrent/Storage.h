#pragma once

#include "Interface2.h"

namespace mtt
{
	class Storage
	{
	public:

		Storage(size_t pieceSize);

		void setPath(std::string path);

		void storePiece(DownloadedPiece& piece);
		void selectFiles(std::vector<mtt::File>& files);
		std::vector<PieceInfo> checkFileHash(mtt::File& file);
		void flush();

		void saveProgress();
		void loadProgress();	

		DownloadSelection selection;
		size_t pieceSize;

	private:

		std::string getFullpath(File& file);
		void storePiece(File& file, DownloadedPiece& piece);

		const int piecesCacheSize = 5;
		void flush(File& file);
		void preallocate(File& file);

		std::string path;
		std::map<int, std::vector<DownloadedPiece>> unsavedPieces;

	};
}
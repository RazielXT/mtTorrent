#pragma once

#include "Interface.h"

namespace mtt
{
	class Storage
	{
	public:

		Storage(uint32_t pieceSize);

		void setPath(std::string path);

		void storePiece(DownloadedPiece& piece);
		PieceBlock getPieceBlock(PieceBlockInfo& piece);

		void setSelection(DownloadSelection& files);
		std::vector<PieceInfo> checkFileHash(mtt::File& file);
		void flush();

		void saveProgress();
		void loadProgress();	

		DownloadSelection selection;
		uint32_t pieceSize;

	private:

		std::string getFullpath(File& file);

		std::vector<PieceBlockInfo> makePieceBlocksInfo(uint32_t index);

		void flush(File& file);
		void preallocate(File& file);

		std::string path;
		std::vector<DownloadedPiece> unsavedPieces;

		struct CachedPiece
		{
			uint32_t index;
			DataBuffer data;
		};
		std::list<CachedPiece> cachedPieces;

		CachedPiece& loadPiece(uint32_t pieceId);
		void loadPiece(File& file, CachedPiece& piece);

	};
}
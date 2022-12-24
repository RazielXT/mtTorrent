#pragma once
#include "Interface.h"
#include <set>

namespace mtt
{
	struct MetadataReconstruction
	{
		void init(uint32_t size);
		bool addPiece(const BufferView& data, uint32_t index);
		bool finished();
		uint32_t getMissingPieceIndex();
		TorrentInfo getRecontructedInfo();

		DataBuffer buffer;
		uint32_t pieces = 0;
		std::set<uint32_t> remainingPieces;

	private:
		uint32_t nextRequestedIndex = 0;
	};
}

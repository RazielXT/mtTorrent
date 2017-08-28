#pragma once
#include "Interface.h"

namespace mtt
{
	struct MetadataReconstruction
	{
		void init(uint32_t size);
		void addPiece(DataBuffer& data, uint32_t index);
		bool finished();
		uint32_t getMissingPieceIndex();
		TorrentInfo getRecontructedInfo();

		DataBuffer buffer;
		uint32_t pieces = 0;
		uint32_t remainingPiecesFlag = -1;
	};
}

#pragma once
#include "Interface.h"

namespace mtt
{
	struct PiecesProgress
	{
		void init(size_t size);

		void select(DownloadSelection& selection);
		void fromBitfield(DataBuffer& bitfield, size_t piecesCount);
		void fromList(std::vector<uint8_t>& pieces);
		DataBuffer toBitfield();

		bool empty();
		float getPercentage();
		float getSelectedPercentage();

		void addPiece(uint32_t index);
		bool hasPiece(uint32_t index);
		bool selectedPiece(uint32_t index);
		uint32_t firstEmptyPiece();

		std::vector<uint8_t> pieces;

	private:

		size_t receivedPiecesCount = 0;
		size_t selectedReceivedPiecesCount = 0;
		size_t selectedPieces = 0;
	};
}

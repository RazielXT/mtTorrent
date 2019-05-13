#pragma once
#include "Interface.h"

namespace mtt
{
	struct PiecesProgress
	{
		void recheckPieces();
		void init(size_t size);
		void resize(size_t size);
		void removeReceived();

		void select(DownloadSelection& selection);
		void fromBitfield(DataBuffer& bitfield);
		void fromList(std::vector<uint8_t>& pieces);
		DataBuffer toBitfield();
		void toBitfield(DataBuffer&);
		size_t getBitfieldSize();

		bool empty();
		float getPercentage();
		float getSelectedPercentage();

		void addPiece(uint32_t index);
		bool hasPiece(uint32_t index);
		bool selectedPiece(uint32_t index);
		bool wantedPiece(uint32_t index);
		uint32_t firstEmptyPiece();

		std::vector<uint8_t> pieces;

	private:

		size_t receivedPiecesCount = 0;
		size_t selectedReceivedPiecesCount = 0;
		size_t selectedPieces = 0;
	};
}

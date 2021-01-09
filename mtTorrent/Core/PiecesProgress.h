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
		bool toBitfield(uint8_t* dataBitfield, size_t dataSize);
		size_t getBitfieldSize();

		bool empty() const;
		float getPercentage() const;
		float getSelectedPercentage() const;

		void addPiece(uint32_t index);
		bool hasPiece(uint32_t index) const;
		void removePiece(uint32_t index);
		bool selectedPiece(uint32_t index) const;
		bool wantedPiece(uint32_t index) const;
		uint32_t firstEmptyPiece() const;
		size_t getReceivedPiecesCount() const;

		std::vector<uint8_t> pieces;
		size_t selectedPieces = 0;

	private:

		size_t receivedPiecesCount = 0;
		size_t selectedReceivedPiecesCount = 0;

	};
}

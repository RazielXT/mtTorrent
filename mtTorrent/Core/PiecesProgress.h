#pragma once

#include "Interface.h"

namespace mtt
{
	struct PiecesProgress
	{
		void calculatePieces();
		void init(size_t size);
		void resize(size_t size);
		void removeReceived(const std::vector<bool>&);

		void select(const File& info, bool selected);
		void fromBitfieldData(const BufferView& bitfield);
		void fromList(const std::vector<uint8_t>& pieces);
		DataBuffer toBitfieldData() const;
		void toBitfield(Bitfield&) const;

		bool empty() const;
		float getPercentage() const;
		float getSelectedPercentage() const;
		bool finished() const;
		bool selectedFinished() const;
		uint64_t getReceivedBytes(uint32_t pieceSize, uint64_t fullSize) const;

		void addPiece(uint32_t index);
		bool hasPiece(uint32_t index) const;
		void removePiece(uint32_t index);
		bool selectedPiece(uint32_t index) const;
		bool wantedPiece(uint32_t index) const;
		size_t getReceivedPiecesCount() const;

		std::vector<uint8_t> pieces;
		size_t selectedPieces = 0;

	private:

		size_t receivedPiecesCount = 0;
		size_t selectedReceivedPiecesCount = 0;

	};
}

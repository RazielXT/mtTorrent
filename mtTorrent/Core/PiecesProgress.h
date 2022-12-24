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
		void select(const TorrentInfo& info, const DownloadSelection& selection);
		void fromBitfield(const BufferView& bitfield);
		void fromList(const std::vector<uint8_t>& pieces);
		DataBuffer toBitfield() const;
		void toBitfield(DataBuffer&) const;
		bool toBitfield(uint8_t* dataBitfield, std::size_t dataSize) const;
		size_t getBitfieldSize() const;

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
		uint32_t firstEmptyPiece() const;
		size_t getReceivedPiecesCount() const;

		std::vector<uint8_t> pieces;
		size_t selectedPieces = 0;

	private:

		size_t receivedPiecesCount = 0;
		size_t selectedReceivedPiecesCount = 0;

	};
}

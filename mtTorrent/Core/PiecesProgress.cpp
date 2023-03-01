#include "PiecesProgress.h"
#include <cmath>

const uint8_t HasFlag = 1;
const uint8_t UnselectedFlag = 8;

bool mtt::PiecesProgress::empty() const
{
	return receivedPiecesCount == 0;
}

float mtt::PiecesProgress::getPercentage() const
{
	return pieces.empty() ? 0 : (receivedPiecesCount / (float)pieces.size());
}

float mtt::PiecesProgress::getSelectedPercentage() const
{
	return selectedPieces == 0 ? 0 : (selectedReceivedPiecesCount / (float)selectedPieces);
}

bool mtt::PiecesProgress::finished() const
{
	return !pieces.empty() && receivedPiecesCount == pieces.size();
}

bool mtt::PiecesProgress::selectedFinished() const
{
	return !pieces.empty() && selectedReceivedPiecesCount == selectedPieces;
}

uint64_t mtt::PiecesProgress::getReceivedBytes(uint32_t pieceSize, uint64_t fullSize) const
{
	if (pieces.empty())
		return 0;

	uint64_t fullPieceCount = receivedPiecesCount;
	uint64_t bytes = 0;

	if (pieces.back() & HasFlag)
	{
		fullPieceCount--;
		bytes = fullSize % pieceSize;
	}

	bytes += fullPieceCount * pieceSize;
	return bytes;
}

void mtt::PiecesProgress::calculatePieces()
{
	receivedPiecesCount = 0;
	selectedReceivedPiecesCount = 0;
	selectedPieces = 0;

	for (auto& p : pieces)
	{
		if (p & HasFlag)
			receivedPiecesCount++;

		if (!(p & UnselectedFlag))
		{
			selectedPieces++;

			if (p & HasFlag)
				selectedReceivedPiecesCount++;
		}
	}
}

void mtt::PiecesProgress::init(size_t size)
{
	receivedPiecesCount = 0;
	selectedReceivedPiecesCount = 0;
	selectedPieces = size;

	if (pieces.size() < size)
	{
		pieces.resize(size, 0);
	}
}

void mtt::PiecesProgress::resize(size_t size)
{
	if (pieces.size() != size)
	{
		auto previous = std::move(pieces);
		pieces.clear();
		pieces.resize(size);

		if (!previous.empty())
			memcpy(pieces.data(), previous.data(), std::min(size, previous.size()));

		calculatePieces();
	}
}

void mtt::PiecesProgress::removeReceived(const std::vector<bool>& where)
{
	for (size_t i = 0; i < pieces.size(); i++)
	{
		if (where[i] && pieces[i] & HasFlag)
		{
			receivedPiecesCount--;
			selectedReceivedPiecesCount--;
			pieces[i] &= ~HasFlag;
		}
	}
}

void mtt::PiecesProgress::select(const File& info, bool selected)
{
	for (uint32_t i = info.startPieceIndex; i <= info.endPieceIndex; i++)
	{
		if (selectedPiece(i) == selected)
			continue;

		if (pieces[i] & HasFlag)
		{
			if (selected)
				selectedReceivedPiecesCount++;
			else
				selectedReceivedPiecesCount--;
		}

		if (selected)
		{
			selectedPieces++;
			pieces[i] &= ~UnselectedFlag;
		}
		else
		{
			selectedPieces--;
			pieces[i] |= UnselectedFlag;
		}
	}
}

void mtt::PiecesProgress::addPiece(uint32_t index)
{
	if (index >= pieces.size())
		resize(index + 1);

	if (!hasPiece(index))
	{
		if (wantedPiece(index))
			selectedReceivedPiecesCount++;

		pieces[index] |= HasFlag;
		receivedPiecesCount++;
	}
}

bool mtt::PiecesProgress::hasPiece(uint32_t index) const
{
	return finished() || pieces[index] & HasFlag;
}

void mtt::PiecesProgress::removePiece(uint32_t index)
{
	if (hasPiece(index))
	{
		if (selectedPiece(index))
			selectedReceivedPiecesCount--;

		pieces[index] &= ~HasFlag;
		receivedPiecesCount--;
	}
}

bool mtt::PiecesProgress::selectedPiece(uint32_t index) const
{
	return (pieces[index] & UnselectedFlag) == 0;
}

bool mtt::PiecesProgress::wantedPiece(uint32_t index) const
{
	return pieces[index] == 0;
}

size_t mtt::PiecesProgress::getReceivedPiecesCount() const
{
	return receivedPiecesCount;
}

void mtt::PiecesProgress::fromBitfieldData(const BufferView& bitfield)
{
	size_t maxPiecesCount = pieces.empty() ? bitfield.size * 8 : pieces.size();

	init(maxPiecesCount);

	for (size_t i = 0; i < maxPiecesCount; i++)
	{
		auto idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield.data[idx] & bitmask) != 0;

		pieces[i] = value ? HasFlag : 0;
		receivedPiecesCount += value;
	}
}

void mtt::PiecesProgress::fromList(const std::vector<uint8_t>& piecesList)
{
	init(piecesList.size());

	for (uint32_t i = 0; i < piecesList.size(); i++)
		if (piecesList[i])
		{
			pieces[i] |= HasFlag;
			receivedPiecesCount++;

			if (selectedPiece(i))
				selectedReceivedPiecesCount++;
		}
		else
			pieces[i] &= ~HasFlag;
}

DataBuffer mtt::PiecesProgress::toBitfieldData() const
{
	Bitfield bitfield;

	toBitfield(bitfield);

	return bitfield.data;
}

void mtt::PiecesProgress::toBitfield(Bitfield& bitfield) const
{
	bitfield.init(pieces.size());

	for (uint32_t i = 0; i < pieces.size(); i++)
	{
		if (!hasPiece(i))
			continue;

		bitfield.put(i);
	}
}

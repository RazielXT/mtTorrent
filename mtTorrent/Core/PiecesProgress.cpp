#include "PiecesProgress.h"

const uint8_t ReadyValue = 0;
const uint8_t HasFlag = 1;
const uint8_t UnselectedFlag = 8;

bool mtt::PiecesProgress::empty()
{
	return pieces.empty();
}

float mtt::PiecesProgress::getPercentage()
{
	return pieces.empty() ? 0 : (receivedPiecesCount / (float)pieces.size());
}

float mtt::PiecesProgress::getSelectedPercentage()
{
	return selectedPieces == 0 ? 0 : (selectedReceivedPiecesCount / (float)selectedPieces);
}

void mtt::PiecesProgress::recheckPieces()
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

	if(pieces.empty())
		selectedPieces = size;

	if (pieces.size() != size)
		pieces.resize(size, 0);
}

void mtt::PiecesProgress::select(DownloadSelection& selection)
{
	init(selection.files.back().info.endPieceIndex + 1);
	selectedPieces = 0;

	uint32_t lastWantedPiece = -1;
	for (auto& f : selection.files)
	{
		uint32_t i = f.info.startPieceIndex;
		if (lastWantedPiece == i)
			i++;

		for (; i <= f.info.endPieceIndex; i++)
		{
			if (pieces[i] & HasFlag)
			{
				receivedPiecesCount++;

				if (f.selected)
					selectedReceivedPiecesCount++;
			}
				
			if (f.selected)
			{
				selectedPieces++;
				pieces[i] &= ~UnselectedFlag;
			}
			else
				pieces[i] |= UnselectedFlag;
		}

		if(f.selected)
			lastWantedPiece = f.info.endPieceIndex;
	}
}

void mtt::PiecesProgress::addPiece(uint32_t index)
{
	if (index >= pieces.size())
		init(index + 1);

	if (!hasPiece(index))
	{
		if (wantedPiece(index))
			selectedReceivedPiecesCount++;

		pieces[index] |= HasFlag;
		receivedPiecesCount++;
	}
}

bool mtt::PiecesProgress::hasPiece(uint32_t index)
{
	return pieces[index] & HasFlag;
}

bool mtt::PiecesProgress::selectedPiece(uint32_t index)
{
	return (pieces[index] & UnselectedFlag) == 0;
}

bool mtt::PiecesProgress::wantedPiece(uint32_t index)
{
	return pieces[index] == 0;
}

uint32_t mtt::PiecesProgress::firstEmptyPiece()
{
	for (uint32_t id = 0; id < pieces.size(); id++)
	{
		if (!pieces[id])
			return id;
	}

	return -1;
}

void mtt::PiecesProgress::fromBitfield(DataBuffer& bitfield, size_t piecesCount)
{
	init(piecesCount);

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;

		pieces[i] = value ? HasFlag : ReadyValue;
		receivedPiecesCount += value;
	}
}

void mtt::PiecesProgress::fromList(std::vector<uint8_t>& piecesList)
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
}

DataBuffer mtt::PiecesProgress::toBitfield()
{
	DataBuffer buffer;
	buffer.resize((size_t)ceil(pieces.size() / 8.0f));

	for (int i = 0; i < pieces.size(); i++)
	{
		if(!hasPiece(i))
			continue;

		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;
		buffer[idx] |= bitmask;
	}

	return buffer;
}

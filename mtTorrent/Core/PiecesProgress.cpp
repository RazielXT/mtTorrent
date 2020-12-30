#include "PiecesProgress.h"

const uint8_t ReadyValue = 0;
const uint8_t HasFlag = 1;
const uint8_t UnselectedFlag = 8;

bool mtt::PiecesProgress::empty()
{
	return receivedPiecesCount == 0;
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

		if(!previous.empty())
			memcpy(pieces.data(), previous.data(), std::min(size, previous.size()));

		recheckPieces();
	}
}

void mtt::PiecesProgress::removeReceived()
{
	receivedPiecesCount = 0;
	selectedReceivedPiecesCount = 0;

	for (auto& p : pieces)
	{
		p &= ~HasFlag;
	}
}

void mtt::PiecesProgress::select(DownloadSelection& selection)
{
	init(selection.files.back().info.endPieceIndex + 1);
	selectedPieces = 0;

	for (size_t i = 0; i < pieces.size(); i++)
	{
		bool selected = std::find_if(selection.files.begin(), selection.files.end(), [i](const FileSelectionInfo& f) 
			{ return f.selected && f.info.startPieceIndex <= i && f.info.endPieceIndex >= i; }) != selection.files.end();

		if (pieces[i] & HasFlag)
		{
			receivedPiecesCount++;

			if (selected)
				selectedReceivedPiecesCount++;
		}

		if (selected)
		{
			selectedPieces++;
			pieces[i] &= ~UnselectedFlag;
		}
		else
			pieces[i] |= UnselectedFlag;
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

bool mtt::PiecesProgress::hasPiece(uint32_t index)
{
	return pieces[index] & HasFlag;
}

void mtt::PiecesProgress::removePiece(uint32_t index)
{
	if (hasPiece(index))
	{
		if (wantedPiece(index))
			selectedReceivedPiecesCount--;

		pieces[index] &= ~HasFlag;
		receivedPiecesCount--;
	}
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

size_t mtt::PiecesProgress::getReceivedPiecesCount()
{
	return receivedPiecesCount;
}

void mtt::PiecesProgress::fromBitfield(DataBuffer& bitfield)
{
	size_t maxPiecesCount = pieces.empty() ? bitfield.size() * 8 : pieces.size();

	init(maxPiecesCount);

	for (size_t i = 0; i < maxPiecesCount; i++)
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
		else
			pieces[i] &= ~HasFlag;
}

DataBuffer mtt::PiecesProgress::toBitfield()
{
	DataBuffer buffer;

	toBitfield(buffer);

	return buffer;
}

bool mtt::PiecesProgress::toBitfield(uint8_t* dataBitfield, size_t dataSize)
{
	if (dataSize < getBitfieldSize())
		return false;

	for (uint32_t i = 0; i < pieces.size(); i++)
	{
		if (!hasPiece(i))
			continue;

		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;
		dataBitfield[idx] |= bitmask;
	}

	return true;
}

void mtt::PiecesProgress::toBitfield(DataBuffer& buffer)
{
	buffer.resize(getBitfieldSize());

	toBitfield(buffer.data(), buffer.size());
}

size_t mtt::PiecesProgress::getBitfieldSize()
{
	return (size_t)ceil(pieces.size() / 8.0f);
}

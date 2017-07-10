#include "PiecesProgress.h"

bool mtt::PiecesProgress::empty()
{
	return pieces.empty();
}

float mtt::PiecesProgress::getPercentage()
{
	return piecesStartCount == 0 ? 1 : (pieces.size() / (float)piecesStartCount);
}

void mtt::PiecesProgress::init(size_t size)
{
	piecesStartCount = size;

	for (uint32_t i = 0; i < piecesStartCount; i++)
	{
		pieces[i] = true;
	}
}

void mtt::PiecesProgress::fromSelection(std::vector<File>& files)
{
	pieces.clear();

	for (auto& f : files)
	{
		for (uint32_t i = f.startPieceIndex; i <= f.endPieceIndex; i++)
		{
			pieces[i] = true;
		}
	}

	piecesStartCount = pieces.size();
}

void mtt::PiecesProgress::addPiece(uint32_t index)
{
	pieces[index] = true;
}

void mtt::PiecesProgress::removePiece(uint32_t index)
{
	auto it = pieces.find(index);

	if (it != pieces.end())
	{
		pieces.erase(it);
	}
}

bool mtt::PiecesProgress::hasPiece(uint32_t index)
{
	auto it = pieces.find(index);
	return it != pieces.end() && it->second;
}

void mtt::PiecesProgress::fromBitfield(DataBuffer& bitfield, size_t piecesCount)
{
	pieces.clear();
	piecesStartCount = piecesCount;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;

		if (value)
			pieces[i] = true;
	}
}

DataBuffer mtt::PiecesProgress::toBitfield()
{
	return{};
}

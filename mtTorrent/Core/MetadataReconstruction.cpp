#include "MetadataReconstruction.h"
#include "MetadataExtension.h"
#include "utils/TorrentFileParser.h"

void mtt::MetadataReconstruction::init(uint32_t sz)
{
	buffer.resize(sz);

	pieces = nextRequestedIndex = 0;
	while (sz > 0)
	{
		remainingPieces.insert(pieces);
		pieces++;

		sz = sz > ext::UtMetadata::PieceSize ? sz - ext::UtMetadata::PieceSize : 0;
	}
}

bool mtt::MetadataReconstruction::addPiece(const BufferView& data, uint32_t index)
{
	auto it = remainingPieces.find(index);
	if (it != remainingPieces.end())
	{
		remainingPieces.erase(it);
		size_t offset = index * ext::UtMetadata::PieceSize;
		memcpy(&buffer[0] + offset, data.data, data.size);
		return true;
	}

	return false;
}

bool mtt::MetadataReconstruction::finished()
{
	return remainingPieces.empty() && pieces != 0;
}

uint32_t mtt::MetadataReconstruction::getMissingPieceIndex()
{
	if (remainingPieces.empty())
		return -1;

	if (nextRequestedIndex > *remainingPieces.rbegin())
		nextRequestedIndex = 0;

	auto it = remainingPieces.lower_bound(nextRequestedIndex);
	nextRequestedIndex++;

	if (it != remainingPieces.end())
		return *it;

	return -1;
}

mtt::TorrentInfo mtt::MetadataReconstruction::getRecontructedInfo()
{
	return mtt::TorrentFileParser::parseTorrentInfo(buffer.data(), buffer.size());
}

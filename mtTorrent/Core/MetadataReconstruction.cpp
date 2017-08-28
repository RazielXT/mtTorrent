#include "MetadataReconstruction.h"
#include "utils\TorrentFileParser.h"

const uint32_t MetadataPieceSize = 16 * 1024;

void mtt::MetadataReconstruction::init(uint32_t sz)
{
	buffer.resize(sz);

	remainingPiecesFlag = pieces = 0;
	uint32_t flagPos = 1;
	while (sz > 0)
	{
		remainingPiecesFlag |= flagPos;
		pieces++;

		flagPos <<= 1;
		sz = sz > MetadataPieceSize ? sz - MetadataPieceSize : 0;
	}
}

void mtt::MetadataReconstruction::addPiece(DataBuffer& data, uint32_t index)
{
	uint32_t flagPos = (uint32_t)(1 << index);
	if (remainingPiecesFlag & flagPos)
	{
		remainingPiecesFlag ^= flagPos;
		size_t offset = index * MetadataPieceSize;
		memcpy(&buffer[0] + offset, data.data(), data.size());
	}
}

bool mtt::MetadataReconstruction::finished()
{
	return remainingPiecesFlag == 0 && pieces != 0;
}

uint32_t mtt::MetadataReconstruction::getMissingPieceIndex()
{
	static uint32_t notRequestedFlag = remainingPiecesFlag;

	uint32_t flag = 1;
	for(uint32_t i = 0; i < pieces; i++)
	{
		if (remainingPiecesFlag & flag && notRequestedFlag & flag)
		{
			notRequestedFlag ^= flag;

			if (notRequestedFlag == 0)
				notRequestedFlag = remainingPiecesFlag;

			return i;
		}

		flag <<= 1;
	}

	return -1;
}

mtt::TorrentInfo mtt::MetadataReconstruction::getRecontructedInfo()
{
	return mtt::TorrentFileParser::parseTorrentInfo(buffer.data(), buffer.size());
}

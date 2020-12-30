#include "Interface.h"
#include "utils/Base32.h"
#include "utils/UrlEncoding.h"
#include "utils/HexEncoding.h"
#include "utils/PacketHelper.h"
#include "utils/BencodeWriter.h"
#include "utils/SHA.h"

using namespace mtt;

bool DownloadedPiece::isValid(const uint8_t* expectedHash)
{
	uint8_t hash[SHA_DIGEST_LENGTH];
	_SHA1((const uint8_t*)data.data(), data.size(), hash);

	return memcmp(hash, expectedHash, SHA_DIGEST_LENGTH) == 0;
}

void mtt::DownloadedPiece::init(uint32_t idx, uint32_t pieceSize, uint32_t blocksCount)
{
	data.resize(pieceSize);
	remainingBlocks = blocksCount;
	blocksState.assign(remainingBlocks, 0);
	index = idx;
}

void DownloadedPiece::addBlock(const PieceBlock& block)
{
	auto blockIdx = (block.info.begin + 1)/ BlockRequestMaxSize;

	if (blockIdx < blocksState.size() && blocksState[blockIdx] == 0)
	{
		memcpy(&data[0] + block.info.begin, block.buffer.data, block.info.length);
		blocksState[blockIdx] = 1;
		remainingBlocks--;
		downloadedSize += block.info.length;
	}
}

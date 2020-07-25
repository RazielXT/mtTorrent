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
	blocksTodo.assign(remainingBlocks, 0);
	index = idx;
}

bool DownloadedPiece::addBlock(PieceBlock& block)
{
	auto blockIdx = (block.info.begin + 1)/ BlockRequestMaxSize;

	if (blockIdx < blocksTodo.size() && blocksTodo[blockIdx] == 0)
	{
		memcpy(&data[0] + block.info.begin, block.buffer.data, block.info.length);
		blocksTodo[blockIdx] = 1;
		remainingBlocks--;
		return true;
	}

	return false;
}

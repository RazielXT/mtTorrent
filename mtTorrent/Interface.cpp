#include "Interface.h"

using namespace mtt;

bool DownloadedPiece::isValid(char* expectedHash)
{
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char*)data.data(), dataSize, hash);

	return memcmp(hash, expectedHash, SHA_DIGEST_LENGTH) == 0;
}

void DownloadedPiece::reset(size_t maxPieceSize)
{
	data.resize(maxPieceSize);
	dataSize = 0;
	receivedBlocks = 0;
	index = -1;
}

void DownloadedPiece::addBlock(PieceBlock& block)
{
	receivedBlocks++;
	dataSize += block.info.length;
	memcpy(&data[0] + block.info.begin, block.data.data(), block.info.length);
}

#include "Interface.h"
#include "utils/Base32.h"
#include "utils/UrlEncoding.h"
#include "utils/HexEncoding.h"

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

static bool parseTorrentHash(std::string& from, uint8_t* to)
{
	if (from.length() == 32)
	{
		auto targetId = base32decode(from);

		if (targetId.length() == 20)
		{
			memcpy(to, targetId.data(), 20);
			return true;
		}
	}
	if (from.length() == 40 && decodeHexa(from, to))
	{
		return true;
	}

	return false;
}

bool mtt::TorrentFileInfo::parseMagnetLink(std::string link)
{
	if (link.length() < 8 || strncmp("magnet:?", link.data(), 8) != 0)
		return false;

	size_t dataPos = 8;
	bool correct = false;

	while (dataPos < link.length())
	{
		auto elementEndPos = link.find_first_of('&', dataPos);
		auto valueStartPos = link.find_first_of('=', dataPos);

		if (valueStartPos < elementEndPos)
		{
			std::string name = link.substr(dataPos, valueStartPos - dataPos);
			std::string value = link.substr(valueStartPos + 1, elementEndPos  - valueStartPos - 1);

			if (name == "xt" && value.length() > 9 && strncmp("urn:btih:", value.data(), 9) == 0)
				correct = parseTorrentHash(value.substr(9), info.hash);
			else if (name == "tr")
			{
				value = UrlDecode(value);

				if (announceList.empty())
					announce = value;

				announceList.push_back(value);
			}
			else if (name == "dn")
			{
				info.name = UrlDecode(value);
			}
		}

		dataPos = elementEndPos;

		if (dataPos != std::string::npos)
			dataPos++;
	}

	if (!correct && link.length() == 32)
		correct = parseTorrentHash(link, info.hash);

	info.expectedBitfieldSize = 0;
	info.pieceSize = 0;
	info.fullSize = 0;

	return correct;
}

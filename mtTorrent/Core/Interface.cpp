#include "Interface.h"
#include "utils/Base32.h"
#include "utils/UrlEncoding.h"
#include "utils/HexEncoding.h"
#include "utils/PacketHelper.h"
#include "utils/BencodeWriter.h"

using namespace mtt;

bool DownloadedPiece::isValid(const uint8_t* expectedHash)
{
	uint8_t hash[SHA_DIGEST_LENGTH];
	SHA1((const uint8_t*)data.data(), data.size(), hash);

	return memcmp(hash, expectedHash, SHA_DIGEST_LENGTH) == 0;
}

void mtt::DownloadedPiece::init(uint32_t idx, uint32_t pieceSize, uint32_t blocksCount)
{
	data.resize(pieceSize);
	remainingBlocks = blocksCount;
	blocksTodo.resize(remainingBlocks, 0);
	index = idx;
}

void DownloadedPiece::addBlock(PieceBlock& block)
{
	auto blockIdx = (block.info.begin + 1)/ BlockRequestMaxSize;

	if (blockIdx < blocksTodo.size() && blocksTodo[blockIdx] == 0)
	{
		memcpy(&data[0] + block.info.begin, block.data.data(), block.info.length);
		blocksTodo[blockIdx] = 1;
		remainingBlocks--;
	}
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

Status mtt::TorrentFileInfo::parseMagnetLink(std::string link)
{
	if (link.length() < 8)
		return Status::E_InvalidInput;

	size_t dataPos = 8;
	bool correct = false;

	info.expectedBitfieldSize = 0;
	info.pieceSize = 0;
	info.fullSize = 0;

	if(strncmp("magnet:?", link.data(), 8) == 0)
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

	if (!correct)
		correct = parseTorrentHash(link, info.hash);

	return correct ? Status::Success : Status::E_InvalidInput;
}

std::string mtt::TorrentFileInfo::createTorrentFileData()
{
	BencodeWriter writer;

	writer.startMap();

	if (!announce.empty())
	{
		writer.addRawItem("8:announce", announce);
	}

	if (!announceList.empty())
	{
		writer.startRawArrayItem("13:announce-list");

		for (auto& a : announceList)
		{
			writer.startArray();
			writer.addText(a);
			writer.endArray();
		}

		writer.endArray();
	}

	writer.startRawMapItem("4:info");

	if (info.files.size() > 1)
	{
		writer.startRawArrayItem("5:files");

		for (auto f : info.files)
		{
			writer.startMap();
			writer.addRawItem("6:length", f.size);
			writer.startRawArrayItem("4:path");
			
			for (size_t i = 1; i < f.path.size(); i++)
			{
				writer.addText(f.path[i]);
			}

			writer.endArray();
			writer.endMap();
		}

		writer.endArray();
	}
	else if (info.files.size() == 1)
		writer.addRawItem("6:length", info.files.front().size);

	writer.addRawItem("4:name", info.name);
	writer.addRawItem("12:piece length", info.pieceSize);

	writer.addRawItemFromBuffer("6:pieces", (const char*)info.pieces.data(), info.pieces.size() * 20);

	writer.endMap();
	writer.endMap();

	return writer.data;
}

std::vector<mtt::PieceBlockInfo> mtt::TorrentInfo::makePieceBlocksInfo(uint32_t index)
{
	std::vector<PieceBlockInfo> out;
	uint32_t size = pieceSize;

	if (index == lastPieceIndex)
		size = lastPieceSize;

	out.reserve(size / BlockRequestMaxSize);

	for (int i = 0; i*BlockRequestMaxSize < size; i++)
	{
		PieceBlockInfo block;
		block.begin = i * BlockRequestMaxSize;
		block.index = index;
		block.length = std::min(size - block.begin, BlockRequestMaxSize);

		out.push_back(block);
	}

	return out;
}

mtt::PieceBlockInfo mtt::TorrentInfo::getPieceBlockInfo(uint32_t idx, uint32_t blockIdx)
{
	PieceBlockInfo block;
	block.begin = blockIdx * BlockRequestMaxSize;
	block.index = idx;
	block.length = BlockRequestMaxSize;

	if (idx == lastPieceIndex && blockIdx == lastPieceLastBlockIndex)
		block.length = lastPieceLastBlockSize;

	return block;
}

uint32_t mtt::TorrentInfo::getPieceSize(uint32_t idx)
{
	return (idx == lastPieceIndex) ? lastPieceSize : pieceSize;
}

uint32_t mtt::TorrentInfo::getPieceBlocksCount(uint32_t idx)
{
	return (idx == lastPieceIndex) ? (lastPieceLastBlockIndex + 1) : (pieceSize / BlockRequestMaxSize);
}

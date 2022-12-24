#include "Interface.h"
#include "utils/Base32.h"
#include "utils/UrlEncoding.h"
#include "utils/HexEncoding.h"
#include "utils/PacketHelper.h"
#include "utils/BencodeWriter.h"
#include "utils/SHA.h"


static bool parseTorrentHash(std::string from, uint8_t* to)
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

	if (from.length() == 44)
	{
		from.erase(std::remove_if(from.begin(), from.end(), isspace), from.end());
	}

	if (from.length() == 40 && decodeHexa(from, to))
	{
		return true;
	}

	return false;
}

std::vector<mtt::PieceBlockInfo> mtt::TorrentInfo::makePieceBlocksInfo(uint32_t index) const
{
	std::vector<PieceBlockInfo> out;
	uint32_t size = pieceSize;

	if (index == lastPieceIndex)
		size = lastPieceSize;

	uint32_t count = getPieceBlocksCount(index);
	out.reserve(count);

	for (uint32_t i = 0; i < count; i++)
	{
		out.push_back(getPieceBlockInfo(index, i));
	}

	return out;
}

mtt::PieceBlockInfo mtt::TorrentInfo::getPieceBlockInfo(uint32_t idx, uint32_t blockIdx) const
{
	PieceBlockInfo block;
	block.begin = blockIdx * BlockRequestMaxSize;
	block.index = idx;
	block.length = std::min(BlockRequestMaxSize, pieceSize - block.begin);

	if (idx == lastPieceIndex && blockIdx == lastPieceLastBlockIndex)
		block.length = lastPieceLastBlockSize;

	return block;
}

uint32_t mtt::TorrentInfo::getPieceSize(uint32_t idx) const
{
	return (idx == lastPieceIndex) ? lastPieceSize : pieceSize;
}

uint32_t mtt::TorrentInfo::getPieceBlocksCount(uint32_t idx) const
{
	return (idx == lastPieceIndex) ? (lastPieceLastBlockIndex + 1) : ((pieceSize + BlockRequestMaxSize - 1) / BlockRequestMaxSize);
}

mtt::Status mtt::TorrentFileInfo::parseMagnetLink(std::string link)
{
	if (link.length() < 8)
		return Status::E_InvalidInput;

	size_t dataPos = 8;
	bool correct = false;

	info.expectedBitfieldSize = 0;
	info.pieceSize = 0;
	info.fullSize = 0;

	if (strncmp("magnet:?", link.data(), 8) == 0)
		while (dataPos < link.length())
		{
			auto elementEndPos = link.find_first_of('&', dataPos);
			auto valueStartPos = link.find_first_of('=', dataPos);

			if (valueStartPos < elementEndPos)
			{
				std::string name = link.substr(dataPos, valueStartPos - dataPos);
				std::string value = link.substr(valueStartPos + 1, elementEndPos - valueStartPos - 1);

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

DataBuffer mtt::TorrentFileInfo::createTorrentFileData(const uint8_t* infoData, std::size_t infoDataSize)
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

	if (!about.createdBy.empty())
		writer.addRawItem("10:created by", about.createdBy);
	if (about.creationDate != 0)
		writer.addRawItem("13:creation date", about.creationDate);

	if (!infoData && !info.data.empty())
	{
		infoData = info.data.data();
		infoDataSize = info.data.size();
	}

	if (!infoData)
	{
		writer.startRawMap("4:info");

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

		if (info.isPrivate)
			writer.addRawItem("7:private", 1);

		writer.endMap();
	}
	else
	{
		writer.data.append("4:info");
		writer.data.append((const char*)infoData, infoDataSize);
	}

	writer.endMap();

	return writer.data;
}

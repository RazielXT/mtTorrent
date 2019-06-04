#include "TorrentFileParser.h"
#include <iostream>
#include <filesystem>
#include "SHA.h"

#define TPARSER_LOG(x) WRITE_LOG(LogTypeFileParser, x)

using namespace mtt;

bool generateInfoHash(BencodeParser& parsed, TorrentFileInfo&);
void loadTorrentFileInfo(BencodeParser& parsed, TorrentFileInfo&);
TorrentInfo parseTorrentInfo(const BencodeParser::Object* info);

TorrentFileInfo TorrentFileParser::parse(const uint8_t* data, size_t length)
{
	TorrentFileInfo out;
	BencodeParser parser;

	if (!parser.parse(data, length))
		return out;
	
	loadTorrentFileInfo(parser, out);
	generateInfoHash(parser, out);

	return out;
}

TorrentFileInfo TorrentFileParser::parseFile(const char* filename)
{
	TorrentFileInfo out;

	size_t maxSize = 10 * 1024 * 1024;
	std::filesystem::path dir(filename);

	if (!std::filesystem::exists(dir) || std::filesystem::file_size(dir) > maxSize)
	{
		TPARSER_LOG("Invalid torrent file " << filename);
		return out;
	}

	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
	{
		TPARSER_LOG("Failed to open torrent file " << filename);
		return out;
	}

	DataBuffer buffer((
		std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	return parse(buffer.data(), buffer.size());
}

static uint32_t getPieceIndex(size_t pos, size_t pieceSize)
{
	size_t p = 0;

	while (pieceSize + p*pieceSize < pos)
	{
		p++;
	}

	return static_cast<uint32_t>(p);
}

void loadTorrentFileInfo(BencodeParser& parser, TorrentFileInfo& fileInfo)
{
	auto root = parser.getRoot();

	if (root && root->isMap())
	{
		if (auto announce = root->getTxtItem("announce"))
			fileInfo.announce = std::string(announce->data, announce->size);

		if (auto list = root->getListItem("announce-list"))
		{
			auto announce = list->getFirstItem();

			while (announce)
			{
				if (announce->isList())
				{
					auto a = announce->getFirstItem();

					while (a)
					{
						if (a->isText())
							fileInfo.announceList.push_back(std::string(a->info.data, a->info.size));

						a = a->getNextSibling();
					}
				}

				announce = announce->getNextSibling();
			}
		}
		else if (!fileInfo.announce.empty())
			fileInfo.announceList.push_back(fileInfo.announce);

		if (auto info = root->getDictItem("info"))
		{
			fileInfo.info = parseTorrentInfo(info);
		}		
	}
}

bool generateInfoHash(BencodeParser& parser, TorrentFileInfo& fileInfo)
{
	const char* infoStart = nullptr;
	const char* infoEnd = nullptr;
	auto dict = parser.getRoot();

	if (dict)
	{
		auto it = dict->getFirstItem();

		while (it)
		{
			if (it->isText("info", 4))
			{
				infoStart = it->info.data + 4;

				auto infod = it->getNextSibling();

				if (infod->isMap())
				{
					auto pieces = infod->getTxtItem("pieces");

					if (pieces)
						infoEnd = pieces->data + pieces->size + 1; 
				}

				it = nullptr;
			}
			else
				it = it->getNextSibling();
		}
	}

	if (infoStart && infoEnd)
	{
		SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&fileInfo.info.hash[0]);

		return true;
	}
	else
		return false;
}

mtt::TorrentInfo mtt::TorrentFileParser::parseTorrentInfo(const uint8_t* data, size_t length)
{
	BencodeParser parser;

	if (parser.parse(data, length))
	{
		auto info = ::parseTorrentInfo(parser.getRoot());

		auto infoStart = (const char*)data;
		auto infoEnd = (const char*)data + length;

		SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&info.hash[0]);

		return info;
	}
	else
		return mtt::TorrentInfo();
}

mtt::TorrentInfo parseTorrentInfo(const BencodeParser::Object* infoDictionary)
{
	mtt::TorrentInfo info;

	info.pieceSize = infoDictionary->getInt("piece length");

	auto piecesHash = infoDictionary->getTxtItem("pieces");

	if (piecesHash && piecesHash->size % 20 == 0)
	{
		PieceInfo temp;
		auto end = piecesHash->data + piecesHash->size;

		for (auto it = piecesHash->data; it != end; it += 20)
		{
			memcpy(temp.hash, it, 20);
			info.pieces.push_back(temp);
		}
	}

	if (auto files = infoDictionary->getListItem("files"))
	{
		info.name = infoDictionary->getTxt("name");

		size_t sizeSum = 0;
		
		auto file = files->getFirstItem();
		while(file)
		{
			std::vector<std::string> path;
			path.push_back(info.name);

			auto pathList = file->getListItem("path");
			auto pathItem = pathList->getFirstItem();
			while (pathItem)
			{
				path.push_back(pathItem->getTxt());
				pathItem = pathItem->getNextSibling();
			}

			size_t size = file->getBigInt("length");
			auto startId = getPieceIndex(sizeSum, info.pieceSize);
			auto startPos = sizeSum % info.pieceSize;
			sizeSum += size;
			auto endId = getPieceIndex(sizeSum, info.pieceSize);
			auto endPos = sizeSum % info.pieceSize;

			info.files.push_back({ path,  size, startId, (uint32_t)startPos, endId, (uint32_t)endPos });
			file = file->getNextSibling();
		}

		info.fullSize = sizeSum;
	}
	else
	{
		size_t size = infoDictionary->getBigInt("length");
		auto endPos = size % info.pieceSize;
		info.name = infoDictionary->getTxt("name");
		info.files.push_back({ { info.name }, size, 0, 0, static_cast<uint32_t>(info.pieces.size() - 1), (uint32_t)endPos });

		info.fullSize = size;
	}

	if (!info.files.empty())
	{
		info.lastPieceIndex = info.files.back().endPieceIndex;
		info.lastPieceSize = info.files.back().endPiecePos;
		info.lastPieceLastBlockIndex = (info.lastPieceSize - 1) / BlockRequestMaxSize;
		info.lastPieceLastBlockSize = info.lastPieceSize - (info.lastPieceLastBlockIndex * BlockRequestMaxSize);
	}

	auto piecesCount = info.pieces.size();
	auto addExpected = piecesCount % 8 > 0 ? 1 : 0; //8 pieces in byte
	info.expectedBitfieldSize = piecesCount / 8 + addExpected;

	return info;
}

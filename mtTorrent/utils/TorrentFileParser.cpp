#include "TorrentFileParser.h"
#include "SHA.h"
#include <iostream>

using namespace mtt;

bool generateInfoHash(BencodeParser& parsed, TorrentFileInfo&, TorrentFileParser::ParsedInfo*);
void loadTorrentFileInfo(BencodeParser& parsed, TorrentFileInfo&);
TorrentInfo parseTorrentInfo(const BencodeParser::Object* info);

TorrentFileInfo TorrentFileParser::parse(const uint8_t* data, size_t length, ParsedInfo* info)
{
	TorrentFileInfo out;
	BencodeParser parser;

	if (!parser.parse(data, length))
		return out;
	
	loadTorrentFileInfo(parser, out);
	generateInfoHash(parser, out, info);

	return out;
}

static uint32_t getPieceIndex(uint64_t pos, uint64_t pieceSize, bool end)
{
	auto idx = (uint32_t) (pos / (uint64_t) pieceSize);

	if (end && (pos % (uint64_t)pieceSize) == 0)
		idx--;

	return idx;
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

		fileInfo.about.createdBy = root->getTxt("created by");
		fileInfo.about.creationDate = root->getBigInt("creation date");
	}
}

bool generateInfoHash(BencodeParser& parser, TorrentFileInfo& fileInfo, TorrentFileParser::ParsedInfo* info)
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
					{
						infoEnd = pieces->data + pieces->size;

						if (*infoEnd != 'e')
						{
							auto endTemp = (infoEnd - 1);
							auto endTempChar = *endTemp;
							*const_cast<char*>(infoEnd - 1) = 'd';

							BencodeParser subparse;
							if (subparse.parse((const uint8_t*)endTemp, parser.bodyEnd - endTemp))
								infoEnd = subparse.bodyEnd;

							*const_cast<char*>(endTemp) = endTempChar;
						}
						else
							infoEnd++;
					}
				}

				it = nullptr;
			}
			else
				it = it->getNextSibling();
		}
	}

	if (infoStart && infoEnd)
	{
		if (info)
		{
			info->infoStart = infoStart;
			info->infoSize = infoEnd - infoStart;
		}

		_SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&fileInfo.info.hash[0]);

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

		_SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&info.hash[0]);

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

	if (auto privateItem = infoDictionary->getIntItem("private"))
	{
		info.isPrivate = privateItem->getInt() != 0;
	}

	if (auto files = infoDictionary->getListItem("files"))
	{
		info.name = infoDictionary->getTxt("name");

		uint64_t sizeSum = 0;
		
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

			uint64_t size = file->getBigInt("length");
			auto startId = getPieceIndex(sizeSum, info.pieceSize, false);
			auto startPos = sizeSum % info.pieceSize;
			sizeSum += size;
			auto endId = size ? getPieceIndex(sizeSum, info.pieceSize, true) : startId;
			auto endPos = sizeSum % info.pieceSize;

			info.files.push_back({ path,  size, startId, (uint32_t)startPos, endId, (uint32_t)endPos });
			file = file->getNextSibling();
		}

		info.fullSize = sizeSum;
	}
	else
	{
		uint64_t size = infoDictionary->getBigInt("length");
		auto endPos = size % info.pieceSize;
		info.name = infoDictionary->getTxt("name");
		info.files.push_back({ { info.name }, size, 0, 0, static_cast<uint32_t>(info.pieces.size() - 1), (uint32_t)(endPos ? endPos : info.pieceSize) });

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

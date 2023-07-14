#include "TorrentFileParser.h"
#include "SHA.h"
#include <iostream>

using namespace mtt;

bool generateInfoHash(BencodeParser& parsed, TorrentFileMetadata&);
void loadTorrentFileInfo(BencodeParser& parsed, TorrentFileMetadata&);
TorrentInfo parseTorrentInfo(const BencodeParser::Object* info);

TorrentFileMetadata TorrentFileParser::parse(const uint8_t* data, std::size_t length)
{
	TorrentFileMetadata out;
	BencodeParser parser;

	if (!parser.parse(data, length))
		return out;
	
	loadTorrentFileInfo(parser, out);
	generateInfoHash(parser, out);

	return out;
}

static uint32_t getPieceIndex(uint64_t pos, uint64_t pieceSize, bool end)
{
	auto idx = (uint32_t) (pos / (uint64_t) pieceSize);

	if (end && (pos % (uint64_t)pieceSize) == 0)
		idx--;

	return idx;
}

void loadTorrentFileInfo(BencodeParser& parser, TorrentFileMetadata& fileInfo)
{
	auto root = parser.getRoot();

	if (root && root->isMap())
	{
		if (auto announce = root->getTxtItem("announce"))
			fileInfo.announce = std::string(announce->data, announce->size);

		if (auto list = root->getListObject("announce-list"))
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
							fileInfo.announceList.emplace_back(a->info.data, a->info.size);

						a = a->getNextSibling();
					}
				}

				announce = announce->getNextSibling();
			}
		}
		else if (!fileInfo.announce.empty())
			fileInfo.announceList.push_back(fileInfo.announce);

		if (auto info = root->getDictObject("info"))
		{
			fileInfo.info = parseTorrentInfo(info);
		}

		fileInfo.about.createdBy = root->getTxt("created by");
		fileInfo.about.creationDate = (Timestamp)root->getBigInt("creation date");
	}
}

bool generateInfoHash(BencodeParser& parser, TorrentFileMetadata& fileInfo)
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
		fileInfo.info.data.assign(infoStart, infoEnd);
		_SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&fileInfo.info.hash[0]);

		return true;
	}

	return false;
}

mtt::TorrentInfo mtt::TorrentFileParser::parseTorrentInfo(const uint8_t* data, std::size_t length)
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

	return {};
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

	info.isPrivate = infoDictionary->getInt("private") != 0;

	if (auto files = infoDictionary->getListObject("files"))
	{
		info.name = infoDictionary->getTxt("name");
		auto namePath = info.paths.insert(info.name).first->c_str();

		uint64_t sizeSum = 0;
		
		auto file = files->getFirstItem();
		while(file)
		{
			std::string filename;

			std::vector<std::string_view> path;
			path.push_back(namePath);

			if (auto pathList = file->getListObject("path"))
			{
				for (auto& p : *pathList)
				{
					if (p.isLastItem())
						filename = p.getTxt();
					else
					{
						auto it = info.paths.insert(p.getTxt());
						path.emplace_back(*it.first);
					}
				}
			}

			uint64_t size = file->getBigInt("length");
			auto startId = getPieceIndex(sizeSum, info.pieceSize, false);
			auto startPos = sizeSum % info.pieceSize;
			sizeSum += size;
			auto endId = size ? getPieceIndex(sizeSum, info.pieceSize, true) : startId;
			auto endPos = sizeSum % info.pieceSize;

			info.files.push_back({ filename, path,  size, startId, (uint32_t)startPos, endId, (uint32_t)endPos });
			file = file->getNextSibling();
		}

		info.fullSize = sizeSum;
	}
	else
	{
		uint64_t size = infoDictionary->getBigInt("length");
		auto endPos = size % info.pieceSize;
		info.name = infoDictionary->getTxt("name");
		info.files.push_back({ info.name, {}, size, 0, 0, static_cast<uint32_t>(info.pieces.size() - 1), (uint32_t)(endPos ? endPos : info.pieceSize) });

		info.fullSize = size;
	}

	if (!info.files.empty())
	{
		info.lastPieceIndex = info.files.back().endPieceIndex;
		info.lastPieceSize = info.files.back().endPiecePos;
		info.lastPieceLastBlockIndex = (info.lastPieceSize - 1) / BlockMaxSize;
		info.lastPieceLastBlockSize = info.lastPieceSize - (info.lastPieceLastBlockIndex * BlockMaxSize);
	}

	return info;
}

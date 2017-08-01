#include "BencodeParser.h"
#include <iostream>
#include <openssl/sha.h>

#define PARSER_LOG(x) {}//WRITE_LOG("PARSER: " << x)

using namespace mtt;

bool BencodeParser::parse(std::string& str)
{
	return parse(str.data(), str.size());
}

bool BencodeParser::parse(DataBuffer& buffer)
{
	return parse((const char*)buffer.data(), buffer.size());
}

bool BencodeParser::parse(const uint8_t* data, size_t length)
{
	return parse((const char*)data, length);
}

bool BencodeParser::parse(const char* data, size_t length)
{
	infoHash.clear();
	infoStart = infoEnd = nullptr;

	bodyEnd = data + length;
	parsedData = internalParse(&data);

	remainingData = bodyEnd - data;
	bodyEnd = data;

	if (infoStart && infoEnd)
	{
		infoHash.resize(SHA_DIGEST_LENGTH);
		SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&infoHash[0]);
	}

	return true;
}

bool BencodeParser::parseFile(const char* filename)
{
	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
	{
		PARSER_LOG("Failed to parse torrent file " << filename << "\n");
		return false;
	}

	DataBuffer buffer((
		std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	return parse(buffer);
}

uint32_t getPieceIndex(size_t pos, size_t pieceSize)
{
	size_t p = 0;

	while (pieceSize + p*pieceSize < pos)
	{
		p++;
	}

	return static_cast<uint32_t>(p);
}

TorrentFileInfo BencodeParser::getTorrentFileInfo()
{
	TorrentFileInfo file;

	if (parsedData.type == Object::Dictionary)
	{
		auto& dictionary = *parsedData.dic;

		if (dictionary.find("announce") != dictionary.end())
			file.announce = dictionary["announce"].txt;

		if (dictionary.find("announce-list") != dictionary.end())
		{
			auto list = dictionary["announce-list"].l;

			for (auto& obj : *list)
			{
				if (obj.type == Object::List)
					for (auto& a : *obj.l)
					{
						if (a.type == Object::Text)
							file.announceList.push_back(a.txt);
					}
			}
		}

		if (dictionary.find("info") != dictionary.end())
		{
			file.info = parseTorrentInfo(*dictionary["info"].dic);
		}		
	}

	return file;
}

#define IS_NUM_CHAR(c) ((c >= '0') && (c <= '9'))

BencodeParser::Object BencodeParser::internalParse(const char** body)
{
	Object obj;
	char c = **body;

	if (IS_NUM_CHAR(c))
	{
		obj = parseString(body);
	}
	else if (c == 'i')
	{
		obj = parseInt(body);
	}
	else if (c == 'l')
	{
		obj = parseList(body);
	}
	else if (c == 'd')
	{
		obj = parseDictionary(body);
	}
	else
		__debugbreak();

	return obj;
}

int deep = 0;

BencodeParser::BenList* BencodeParser::parseList(const char** body)
{
	(*body)++;

	BenList* list = new BenList();

	PARSER_LOG("List start" << std::to_string(deep) << "\n");

	deep++;

	while (**body != 'e' && (*body < bodyEnd))
	{
		list->push_back(internalParse(body));
	}

	(*body)++;

	deep--;

	PARSER_LOG("List end" << std::to_string(deep) << "\n");

	return list;
}

BencodeParser::BenDictionary* BencodeParser::parseDictionary(const char** body)
{
	(*body)++;

	BenDictionary* dic = new BenDictionary();

	PARSER_LOG("Dictionary start" << std::to_string(deep) << "\n");

	deep++;

	while (**body != 'e' && (*body < bodyEnd))
	{
		auto key = internalParse(body);

		if (key.type != Object::Text)
			throw;

		if (key.txt == "info")
			infoStart = *body;

		auto value = internalParse(body);

		if (key.txt == "info")
			infoEnd = *body;

		(*dic)[key.txt] = value;
	}

	deep--;

	(*body)++;

	PARSER_LOG("Dictionary end" << std::to_string(deep) << "\n");

	return dic;
}

std::string BencodeParser::parseString(const char** body)
{
	std::string len;
	while (IS_NUM_CHAR(**body))
	{
		len += (**body);
		(*body)++;
	}

	if (**body != ':')
		throw;

	(*body)++;

	int length = std::stoi(len);

	std::string out(*body,*body + length);

	(*body) += length;

	if(out.length()<100)
		PARSER_LOG("String: " << out << "\n");

	if (out == "pieces")
		length++;

	return out;
}

int BencodeParser::parseInt(const char** body)
{
	//i
	(*body)++;

	std::string num;
	
	while (IS_NUM_CHAR(**body) || (**body == '-' && num.empty()))
	{
		num += (**body);
		(*body)++;
	}

	if (**body != 'e')
		throw;

	(*body)++;

	PARSER_LOG("Integer: " << num << "\n");

	return std::stoi(num);
}

mtt::BencodeParser::~BencodeParser()
{
	parsedData.cleanup();
}

mtt::TorrentInfo mtt::BencodeParser::parseTorrentInfo(const char* data, size_t length)
{
	parse(data, length);

	if (parsedData.dic)
	{
		infoStart = data;
		infoEnd = data + length;
		infoHash.clear();

		return parseTorrentInfo(*parsedData.dic);
	}		
	else
		return mtt::TorrentInfo();
}

mtt::TorrentInfo mtt::BencodeParser::parseTorrentInfo(const uint8_t* data, size_t length)
{
	return parseTorrentInfo((const char*)data, length);
}

mtt::TorrentInfo mtt::BencodeParser::parseTorrentInfo(BenDictionary& infoDictionary)
{
	mtt::TorrentInfo info;

	info.pieceSize = infoDictionary["piece length"].i;

	auto& piecesHash = infoDictionary["pieces"].txt;

	if (piecesHash.size() % 20 == 0)
	{
		PieceInfo temp;
		auto end = piecesHash.data() + piecesHash.size();

		for (auto it = piecesHash.data(); it != end; it += 20)
		{
			memcpy(temp.hash, it, 20);
			info.pieces.push_back(temp);
		}
	}

	if (infoDictionary.find("files") != infoDictionary.end())
	{
		info.directory = infoDictionary["name"].txt;

		size_t sizeSum = 0;
		auto& files = *infoDictionary["files"].l;

		int i = 0;
		for (auto& f : files)
		{
			std::vector<std::string> path;
			path.push_back(info.directory);

			auto& pathList = *(*f.dic)["path"].l;
			for (auto& p : pathList)
			{
				path.push_back(p.txt);
			}

			size_t size = (*f.dic)["length"].i;
			auto startId = getPieceIndex(sizeSum, info.pieceSize);
			auto startPos = sizeSum % info.pieceSize;
			sizeSum += size;
			auto endId = getPieceIndex(sizeSum, info.pieceSize);
			auto endPos = sizeSum % info.pieceSize;

			info.files.push_back({ i++, path,  size, startId, startPos, endId, endPos });
		}

		info.fullSize = sizeSum;
	}
	else
	{
		size_t size = infoDictionary["length"].i;
		auto endPos = size % info.pieceSize;
		info.files.push_back({ 0,{ infoDictionary["name"].txt }, size, 0, 0, static_cast<uint32_t>(info.pieces.size() - 1), endPos });

		info.fullSize = size;
	}

	auto piecesCount = info.pieces.size();
	auto addExpected = piecesCount % 8 > 0 ? 1 : 0; //8 pieces in byte
	info.expectedBitfieldSize = piecesCount / 8 + addExpected;

	if (infoStart && infoEnd)
	{
		if (infoHash.empty())
			SHA1((const unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&info.hash[0]);
		else
			memcpy(&info.hash[0], infoHash.data(), SHA_DIGEST_LENGTH);
	}

	return info;
}

BencodeParser::BenList* BencodeParser::Object::getListItem(const char* name)
{
	if (type == BencodeParser::Object::Dictionary)
	{
		if (dic->find(name) != dic->end())
		{
			auto& item = (*dic)[name];

			if(item.type == BencodeParser::Object::List)
				return item.l;
		}
	}

	return nullptr;
}

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getDictObject(const char* name)
{
	if (type == BencodeParser::Object::Dictionary)
	{
		if (dic->find(name) != dic->end())
		{
			auto& item = (*dic)[name];

			if (item.type == BencodeParser::Object::Dictionary)
				return &item;
		}
	}

	return nullptr;
}

BencodeParser::BenDictionary* mtt::BencodeParser::Object::getDictItem(const char* name)
{
	if (type == BencodeParser::Object::Dictionary)
	{
		if (dic->find(name) != dic->end())
		{
			auto& item = (*dic)[name];

			if (item.type == BencodeParser::Object::Dictionary)
				return item.dic;
		}
	}

	return nullptr;
}

std::string* mtt::BencodeParser::Object::getTxtItem(const char* name)
{
	if (type == BencodeParser::Object::Dictionary)
	{
		if (dic->find(name) != dic->end())
		{
			auto& item = (*dic)[name];

			if (item.type == BencodeParser::Object::Text)
				return &item.txt;
		}
	}

	return nullptr;
}

int* mtt::BencodeParser::Object::getIntItem(const char* name)
{
	if (type == BencodeParser::Object::Dictionary)
	{
		if (dic->find(name) != dic->end())
		{
			auto& item = (*dic)[name];

			if (item.type == BencodeParser::Object::Number)
				return &item.i;
		}
	}

	return nullptr;
}

bool mtt::BencodeParser::Object::isMap()
{
	return type == Dictionary;
}

bool mtt::BencodeParser::Object::isList()
{
	return type == List;
}

bool mtt::BencodeParser::Object::isInt()
{
	return type == Number;
}

void mtt::BencodeParser::Object::cleanup()
{
	if (type == Dictionary)
	{
		for (auto& o : *dic)
		{
			o.second.cleanup();
		}

		delete dic;
	}

	if (type == List)
	{
		for (auto& o : *l)
		{
			o.cleanup();
		}

		delete l;
	}

	type = None;
}

#include "BencodeParser.h"
#include <iostream>
#include <openssl/sha.h>

using namespace Torrent;

bool BencodeParser::parse(std::vector<char>& buffer)
{
	char* data = &buffer[0];
	bodyEnd = data + buffer.size();
	parsedData = parse(&data);

	return true;
}

bool BencodeParser::parseFile(char* filename)
{
	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
	{
		std::cout << "Failed to parse torrent file " << filename << "\n";
		return false;
	}

	std::vector<char> buffer((
		std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	return parse(buffer);
}

size_t getPieceIndex(size_t pos, size_t pieceSize)
{
	size_t p = 0;

	while (p*pieceSize < pos)
	{
		p++;
	}

	return p;
}

TorrentInfo BencodeParser::parseTorrentInfo()
{
	TorrentInfo info;

	if (parsedData.type == Object::Dictionary)
	{
		auto& dictionary = *parsedData.dic;

		if (dictionary.find("announce") != dictionary.end())
			info.announce = dictionary["announce"].txt;

		if (dictionary.find("announce-list") != dictionary.end())
		{
			auto list = dictionary["announce-list"].l;

			for (auto& obj : *list)
			{
				if (obj.type == Object::List)
					for (auto& a : *obj.l)
					{
						if (a.type == Object::Text)
							info.announceList.push_back(a.txt);
					}
			}
		}

		if (dictionary.find("info") != dictionary.end())
		{
			auto& infoDictionary = *dictionary["info"].dic;

			info.pieceSize = infoDictionary["piece length"].i;

			auto& pieces = infoDictionary["pieces"].txt;
			
			if (pieces.size() % 20 == 0)
			{
				PieceObj temp;
				auto end = pieces.data() + pieces.size();

				for (auto it = pieces.data(); it != end; it += 20)
				{
					memcpy(&temp.hash, it, 20);
					info.pieces.push_back(temp);
				}
			}

			if (infoDictionary.find("files") != infoDictionary.end())
			{
				info.directory = infoDictionary["name"].txt;

				size_t sizeSum = 0;
				auto& files = *infoDictionary["files"].l;

				for (auto& f : files)
				{
					std::string path;

					auto& pathList = *(*f.dic)["path"].l;
					for (auto& p : pathList)
					{
						if (!path.empty()) path += "//";
						path += p.txt;
					}

					size_t size = (*f.dic)["length"].i;
					auto startId = getPieceIndex(sizeSum, info.pieceSize);
					auto startPos = sizeSum % info.pieceSize;
					sizeSum += size;
					auto endId = getPieceIndex(sizeSum, info.pieceSize);
					auto endPos = sizeSum % info.pieceSize;

					info.files.push_back({ path,  size, startId, startPos, endId, endPos});
				}

				if (sizeSum != 0)
					sizeSum++;
			}
			else
			{
				size_t size = infoDictionary["length"].i;
				auto endPos = size % info.pieceSize;
				info.files.push_back({infoDictionary["name"].txt, size, 0, 0, pieces.size() - 1, endPos});
			}

			auto piecesNum = pieces.size() / 20;
			auto addExpected = piecesNum % 8 > 0 ? 1 : 0; //8 pieces in byte
			info.expectedBitfieldSize = piecesNum / 8 + addExpected;
		}		
	}
	
	if (!infoStart || !infoEnd)
		return info;

	info.infoHash.resize(SHA_DIGEST_LENGTH);

	SHA1((unsigned char*)infoStart, infoEnd - infoStart, (unsigned char*)&info.infoHash[0]);

	return info;
}

#define IS_NUM_CHAR(c) ((c >= '0') && (c <= '9'))

BencodeParser::Object BencodeParser::parse(char** body)
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

BencodeParser::BenList* BencodeParser::parseList(char** body)
{
	//l
	(*body)++;

	BenList* list = new BenList();

	std::cout << "List start" << std::to_string(deep) << "\n";

	deep++;

	while (**body != 'e' && (*body < bodyEnd))
	{
		list->push_back(parse(body));
	}

	(*body)++;

	deep--;

	std::cout << "List end" << std::to_string(deep) << "\n";

	return list;
}

BencodeParser::BenDictionary* BencodeParser::parseDictionary(char** body)
{
	//d
	(*body)++;

	BenDictionary* dic = new BenDictionary();

	std::cout << "Dictionary start" << std::to_string(deep) << "\n";

	deep++;

	while (**body != 'e' && (*body < bodyEnd))
	{
		auto key = parse(body);

		if (key.type != Object::Text)
			throw;

		if (key.txt == "info")
			infoStart = *body;

		auto value = parse(body);

		if (key.txt == "info")
			infoEnd = *body;

		(*dic)[key.txt] = value;
	}

	deep--;

	(*body)++;

	std::cout << "Dictionary end" << std::to_string(deep) << "\n";

	return dic;
}

std::string BencodeParser::parseString(char** body)
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
		std::cout << "String: " << out << "\n";

	if (out == "pieces")
		length++;

	return out;
}

int BencodeParser::parseInt(char** body)
{
	//i
	(*body)++;

	std::string num;
	
	while (IS_NUM_CHAR(**body))
	{
		num += (**body);
		(*body)++;
	}

	if (**body != 'e')
		throw;

	(*body)++;

	std::cout << "Integer: " << num << "\n";

	return std::stoi(num);
}

Torrent::BencodeParser::~BencodeParser()
{
	parsedData.cleanup();
}

void Torrent::BencodeParser::Object::cleanup()
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

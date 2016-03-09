#include "TorrentFileParser.h"
#include <iostream>
#include <openssl/sha.h>

bool TorrentFileParser::parse(char* filename)
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

	char* data = &buffer[0];
	bodyEnd = data + buffer.size();

	parsedData = parse(&data);

	parseTorrentInfo();

	return true;
}

void TorrentFileParser::parseTorrentInfo()
{
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
				TorrentFileInfo::PieceObj temp;
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

					info.files.push_back({ path, (*f.dic)["length"].i });
				}
			}
			else
			{
				info.files.push_back({infoDictionary["name"].txt, infoDictionary["length"].i});
			}

			auto piecesNum = pieces.size() / 20;
			auto addExpected = piecesNum % 8 > 0 ? 1 : 0; //8 pieces in byte
			info.expectedBitfieldSize = piecesNum / 8 + addExpected;
		}		
	}

	computeInfoHash();
}

void TorrentFileParser::computeInfoHash()
{
	if (!infoStart || !infoEnd)
		return;

	info.infoHash.resize(SHA_DIGEST_LENGTH);

	SHA1((unsigned char*) infoStart, infoEnd - infoStart, (unsigned char*)&info.infoHash[0]);
}

#define IS_NUM_CHAR(c) ((c >= '0') && (c <= '9'))

TorrentFileParser::Object TorrentFileParser::parse(char** body)
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

TorrentFileParser::TorrentList* TorrentFileParser::parseList(char** body)
{
	//l
	(*body)++;

	TorrentList* list = new TorrentList();

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

TorrentFileParser::TorrentDictionary* TorrentFileParser::parseDictionary(char** body)
{
	//d
	(*body)++;

	TorrentDictionary* dic = new TorrentDictionary();

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

std::string TorrentFileParser::parseString(char** body)
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

int TorrentFileParser::parseInt(char** body)
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

#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>

struct TorrentFileInfo
{
	std::string announce;
	std::vector<std::string> announceList;

	std::vector<char> infoHash;
	
	struct PieceObj
	{
		char hash[20];
	};
	std::vector<PieceObj> pieces;
	size_t pieceSize;
	size_t expectedBitfieldSize;

	struct File
	{
		std::string name;
		size_t size;
		size_t startPieceIndex;
		size_t startPiecePos;
		size_t endPieceIndex;
		size_t endPiecePos;
	};
	std::vector<File> files;
	std::string directory;
};

class TorrentFileParser
{
public:

	bool parse(char* filename);

	struct Object;

	using TorrentList = std::vector<Object>;
	using TorrentDictionary = std::map<std::string, Object>;

	struct Object
	{
		//union
		//{
			int i;
			std::string txt;
			TorrentList* l;
			TorrentDictionary* dic;
		//};

		enum Type { None, Number, Text, List, Dictionary } type;

		Object() { type = None; }
		Object(Object& r) { type = r.type; txt = r.txt; i = r.i; l = r.l; dic = r.dic; r.type = None; }
		//~Object() { if (type == List) delete l; if (type == Dictionary) delete dic; }
		Object(int number) { i = number; type = Number; }
		Object(std::string text) { txt = text; type = Text; }
		Object(TorrentList* list) { l = list; type = List; }
		Object(TorrentDictionary* dictionary) { dic = dictionary; type = Dictionary; }
	};

	Object parsedData;
	
	TorrentFileInfo info;

private:

	void parseTorrentInfo();

	void computeInfoHash();

	Object parse(char** body);

	TorrentList* parseList(char** body);
	TorrentDictionary* parseDictionary(char** body);
	std::string parseString(char** body);
	int parseInt(char** body);

	char* bodyEnd = nullptr;
	char* infoStart = nullptr;
	char* infoEnd = nullptr;

	enum ParseState {} state;
};
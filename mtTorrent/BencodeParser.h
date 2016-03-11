#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "Interface.h"

namespace Torrent
{
	class BencodeParser
	{
	public:

		~BencodeParser();

		bool parseFile(char* filename);
		bool parse(std::vector<char>& buffer);

		struct Object;

		using BenList = std::vector<Object>;
		using BenDictionary = std::map<std::string, Object>;

		struct Object
		{
			//union
			//{
			int i;
			std::string txt;
			BenList* l;
			BenDictionary* dic;
			//};

			enum Type { None, Number, Text, List, Dictionary } type;

			Object() { type = None; }
			Object(Object& r) { type = r.type; txt = r.txt; i = r.i; l = r.l; dic = r.dic; r.type = None; }
			Object(int number) { i = number; type = Number; }
			Object(std::string text) { txt = text; type = Text; }
			Object(BenList* list) { l = list; type = List; }
			Object(BenDictionary* dictionary) { dic = dictionary; type = Dictionary; }

			void cleanup();
		};

		Object parsedData;

		TorrentInfo parseTorrentInfo();

	private:

		Object parse(char** body);

		BenList* parseList(char** body);
		BenDictionary* parseDictionary(char** body);
		std::string parseString(char** body);
		int parseInt(char** body);

		char* bodyEnd = nullptr;
		char* infoStart = nullptr;
		char* infoEnd = nullptr;

	};

}

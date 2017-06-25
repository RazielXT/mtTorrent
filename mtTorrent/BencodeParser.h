#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "TorrentDefines.h"

namespace mtt
{
	class BencodeParser
	{
	public:

		~BencodeParser();

		bool parseFile(const char* filename);
		bool parse(DataBuffer& buffer);
		bool parse(std::string& str);
		bool parse(const char* data, size_t length);

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

			BenList* getListItem(const char* name);
			std::string* getTxtItem(const char* name);
			int* getIntItem(const char* name);
			bool isMap();
		};

		Object parsedData;
		DataBuffer infoHash;

		TorrentFileInfo parseTorrentInfo();

	private:

		Object internalParse(const char** body);

		BenList* parseList(const char** body);
		BenDictionary* parseDictionary(const char** body);
		std::string parseString(const char** body);
		int parseInt(const char** body);

		const char* bodyEnd = nullptr;
		const char* infoStart = nullptr;
		const char* infoEnd = nullptr;

	};

}

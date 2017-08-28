#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace mtt
{
	class BencodeParser
	{
	public:

		bool parse(const uint8_t* data, size_t length);
		
		struct Object
		{
			struct Item
			{
				const char* data = nullptr;
				int size = 0;

				uint16_t nextSiblingOffset = 0;
				enum Type : uint16_t { None, Number, Text, List, Dictionary } type = None;

				bool equals(const char*, size_t);
			}
			info;
		
			Object* getNextSibling();

			Object* getDictItem(const char* name);
			Object* getListItem(const char* name);
			Item* getTxtItem(const char* name);
			std::string getTxt(const char* name);
			std::string getTxt();
			Object* getIntItem(const char* name);
			int getInt(const char* name);
			int getInt();

			Object* getFirstItem();

			bool isMap();
			bool isList();
			bool isInt();
			bool isText();
			bool isText(const char*, size_t);
		};

		Object* getRoot();

		const char* bodyEnd = nullptr;
		size_t remainingData = 0;

		const char* infoStart = nullptr;
		const char* infoEnd = nullptr;

	protected:

		std::vector<Object> objects;
		int deep = 0;

		Object* internalParse(const char** body);
		Object* parseList(const char** body);
		Object* parseDictionary(const char** body);
		Object* parseString(const char** body);
		Object* parseInt(const char** body);

		void parseError(const char** body);
	};

}

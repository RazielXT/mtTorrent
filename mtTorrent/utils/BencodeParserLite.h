#pragma once
#include <cstdint>
#include <vector>

namespace mtt
{
	class BencodeParserLite
	{
	public:

		~BencodeParserLite();

		bool parse(const uint8_t* data, size_t length);
		
		struct Object
		{
			struct Item
			{
				const char* data = nullptr;
				int size = 0;

				uint16_t nextSiblingOffset = 0;
				enum Type : uint8_t { None, Number, Text, List, Dictionary } type = None;
			}
			info;
		
			Object* getNextSibling();

			Object* getDictItem(const char* name);
			Object* getListItem(const char* name);
			Item* getTxtItem(const char* name);
			Object* getNumItem(const char* name);
			int getInt(const char* name);
			int getInt();

			Object* getFirstItem();

			bool isMap();
			bool isList();
			bool isInt();
			bool isText();

			bool equals(const char*, size_t);
		};

		Object* getRoot();

	protected:

		std::vector<Object> objects;
		int deep = 0;
		const char* bodyEnd = nullptr;

		Object* internalParse(const char** body);
		Object* parseList(const char** body);
		Object* parseDictionary(const char** body);
		Object* parseString(const char** body);
		Object* parseInt(const char** body);

		void parseError(const char** body);
	};

}

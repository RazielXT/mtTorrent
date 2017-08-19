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
			enum Type { Type_None, Type_Number, Type_Text, Type_List, Type_Dictionary } type = Type_None;

			struct Text
			{
				const char* data;
				int length;
			};

			struct Number
			{
				const char* data;
				int length;
			};

			struct List
			{
				int size;
			};

			struct Dictionary
			{
				int size;
			};

			int nextSiblingOffset = 0;
			Object* getNextSibling();

			union
			{
				Text text;
				Number number;
				List list;
				Dictionary map;
			}
			data;

			std::vector<Object>* objects;

			Object* getDictItem(const char* name);
			Object* getListItem(const char* name);
			Text* getTxtItem(const char* name);
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

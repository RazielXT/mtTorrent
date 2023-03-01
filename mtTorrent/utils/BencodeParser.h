#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace mtt
{
	class BencodeParser
	{
	public:

		bool parse(const uint8_t* data, std::size_t length);
		
		struct Object
		{
			struct Item
			{
				const char* data = nullptr;
				int size = 0;

				uint16_t nextSiblingOffset = 0;
				enum Type : uint16_t { None, Number, Text, List, Dictionary } type = None;

				bool equals(const char*, std::size_t) const;
				std::string toString() const;
			}
			info;

			const Object* getDictObject(const char* name) const;
			const Object* getListObject(const char* name) const;
			const Item* getTxtItem(const char* name) const;
			const Item* popTxtItem(const char* name);
			std::string getTxt(const char* name) const;
			std::string getTxt() const;
			const Object* getIntObject(const char* name) const;
			int getInt(const char* name) const;
			int getInt() const;
			uint64_t getBigInt(const char* name) const;
			uint64_t getBigInt() const;

			template <typename T>
			T getValueOr(const char* name, T def) const
			{
				auto o = getIntObject(name);
				return o ? (T)o->getInt() : def;
			}

			const Object* getFirstItem() const;
			const Object* getNextSibling() const;
			bool isLastItem() const;

			struct iterator {
			public:
				iterator(const Object* ptr, bool map) : ptr(ptr), isMap(map){}
				iterator operator++() { ptr = ptr->getNextSibling(); if (isMap) ptr = ptr->getNextSibling(); return *this; }
				bool operator!=(const iterator& other) const { return ptr != other.ptr; }
				const Object& operator*() const { return *ptr; }
			private:
				const Object* ptr;
				bool isMap;
			};

			iterator begin() const { return { getFirstItem(), isMap()}; };
			iterator end() const { return { nullptr, false }; };

			bool isMap() const;
			bool isList() const;
			bool isInt() const;
			bool isText() const;
			bool isText(const char*, std::size_t) const;

			const Object& getDictItemValue() const;
		};

		Object* getRoot();

		const char* bodyStart = nullptr;
		const char* bodyEnd = nullptr;
		size_t remainingData = 0;

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

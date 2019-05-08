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

				bool equals(const char*, size_t) const;
			}
			info;

			const Object* getDictItem(const char* name) const;
			const Object* getListItem(const char* name) const;
			const Item* getTxtItem(const char* name) const;
			std::string getTxt(const char* name) const;
			std::string getTxt() const;
			const Object* getIntItem(const char* name) const;
			int getInt(const char* name) const;
			int getInt() const;
			size_t getBigInt(const char* name) const;
			size_t getBigInt() const;

			template <typename T>
			T getValueOr(const char* name, T def) const
			{
				auto o = getIntItem(name);
				return o ? (T)o->getInt() : def;
			}
			template <>
			std::string getValueOr(const char* name, std::string def) const
			{
				auto item = getTxtItem(name);
				return item ? std::string(item->data, item->size) : def;
			}

			const Object* getFirstItem() const;
			const Object* getNextSibling() const;

			struct iterator {
			public:
				iterator(const Object* ptr) : ptr(ptr) {}
				iterator operator++() { ptr = ptr->getNextSibling(); return *this; }
				bool operator!=(const iterator& other) const { return ptr != other.ptr; }
				const Object& operator*() const { return *ptr; }
			private:
				const Object* ptr;
			};

			iterator begin() const { return iterator(getFirstItem()); };
			iterator end() const { return iterator(nullptr); };

			bool isMap() const;
			bool isList() const;
			bool isInt() const;
			bool isText() const;
			bool isText(const char*, size_t) const;
		};

		Object* getRoot();

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

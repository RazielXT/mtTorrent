#pragma once

#include <string>
#include <vector>

namespace mtt
{
	namespace XmlParser
	{
		struct Element
		{
			std::string_view name;
			enum class Type : uint8_t { Node, String, Attribute, Cdata } type;

			std::string_view value() const;
			std::string_view value(std::string_view name) const;
			uint64_t valueNumber() const;
			uint64_t valueNumber(std::string_view name) const;
			bool hasValue = false;

			const Element* nextSibling() const;
			const Element* nextSibling(std::string_view name) const;
			uint16_t nextSiblingOffset = 0;

			const Element* firstNode() const;
			const Element* firstNode(std::string_view name) const;
			uint16_t firstChildOffset = 0;

			const Element* attributes() const;
			uint8_t attributesCount = 0;

			struct iterator {
			public:
				iterator(const Element* ptr) : ptr(ptr) {}
				iterator operator++() { ptr = ptr->nextSibling(); return *this; }
				bool operator!=(const iterator& other) const { return ptr != other.ptr; }
				const Element& operator*() const { return *ptr; }
			private:
				const Element* ptr;
			};

			iterator begin() const { return {firstNode()}; };
			iterator end() const { return {nullptr}; };
		};

		struct Document
		{
			const Element* getRoot();

			bool parse(const char* input, size_t size);

		private:
			std::vector<Element> elements;
		};
	};

	namespace XmlWriter
	{
		struct Element
		{
			Element(std::string& buffer, std::string_view name);

			Element(Element& parent, std::string_view name);

			Element(Element& parent, std::string_view name, std::initializer_list<std::pair<std::string_view, std::string_view>> attributes);

			Element createChild(std::string_view);

			Element& setValue(std::string_view);

			void addValueCData(std::string_view, std::string_view value);

			void addValue(std::string_view, std::string_view value);

			void addValue(std::string_view, uint64_t value);

			void close();

		private:
			size_t depth = 0;
			bool hasChildren = false;
			std::string& buffer;
			std::string name;
		};
	}
};
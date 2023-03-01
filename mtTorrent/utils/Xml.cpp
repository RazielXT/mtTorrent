#include "Xml.h"
#include <functional>
#include <cctype>
#include <cstring>
#include <algorithm>

const mtt::XmlParser::Element* mtt::XmlParser::Document::getRoot()
{
	return elements.empty() ? nullptr : &elements.front();
}

/*
variation of parsing algorithm from libtorrent xml_parserXml
*/
bool mtt::XmlParser::Document::parse(const char* input, size_t size)
{
	elements.reserve(std::min((size_t)100, size / 25));

	char const* p = input;
	char const* end = input + size;

	std::vector<size_t> currentTreeStack;
	size_t prevSibling = 0;

	for (; p != end; ++p)
	{
		char const* start = p;
		for (; p != end && *p != '<'; ++p);

		if (p != start)
		{
			if (!currentTreeStack.empty())
			{
				elements[currentTreeStack.back()].hasValue = true;
				elements.push_back({ { start, std::size_t(p - start) }, Element::Type::String });
			}
		}

		if (p == end) break;

		// '<'
		++p;
		if (p != end && p + 8 < end && std::memcmp(p, "![CDATA[", 8) == 0)
		{
			p += 8;
			start = p;
			while (p != end && std::memcmp(p - 2, "]]>", 3) != 0) ++p;

			// parse error
			if (p == end)
			{
				return false;
			}

			elements.push_back({ { start, std::size_t(p - start - 2) }, Element::Type::Cdata });
			continue;
		}

		// parse the name of the tag.
		for (start = p; p != end && *p != '>' && !isspace(*p); ++p);

		char const* tag_name_end = p;

		// skip the attributes for now
		for (; p != end && *p != '>'; ++p);

		// parse error
		if (p == end)
		{
			return false;
		}

		char const* tag_end = p;
		if (*start == '/')
		{
			++start;
			if (!currentTreeStack.empty())
			{
				prevSibling = currentTreeStack.back();
				currentTreeStack.pop_back();
			}
		}
		else if (*(p - 1) == '/')
		{
			//doc.elements.push_back({ { start, std::size_t(std::min(tag_name_end - start, p - start - 1)) }, Element::Type::Node });
			tag_end = p - 1;
		}
		else if (*start == '?' && *(p - 1) == '?')
		{
			++start;
			//doc.declaration = { start, std::size_t(std::min(tag_name_end - start, p - start - 1)) };
			tag_end = p - 1;
		}
		else if (start + 5 < p && std::memcmp(start, "!--", 3) == 0 && std::memcmp(p - 2, "--", 2) == 0)
		{
			start += 3;
			continue;
		}
		else
		{
			if (prevSibling)
			{
				elements[prevSibling].nextSiblingOffset = (uint16_t)(elements.size() - prevSibling);
				prevSibling = 0;
			}
			if (!currentTreeStack.empty() && elements[currentTreeStack.back()].firstChildOffset == 0)
			{
				elements[currentTreeStack.back()].firstChildOffset = (uint16_t)(elements.size() - currentTreeStack.back());
			}
			currentTreeStack.push_back(elements.size());
			elements.push_back({ { start, std::size_t(tag_name_end - start) }, Element::Type::Node });
		}

		// parse attributes
		for (char const* i = tag_name_end; i < tag_end; ++i)
		{
			char const* val_start = nullptr;

			// find start of attribute name
			while (i != tag_end && isspace(*i)) ++i;
			if (i == tag_end) break;
			start = i;
			// find end of attribute name
			while (i != tag_end && *i != '=' && !isspace(*i)) ++i;
			auto const name_len = std::size_t(i - start);

			// look for equality sign
			for (; i != tag_end && *i != '='; ++i);

			// no equality sign found
			if (i == tag_end)
			{
				break;
			}

			++i;
			while (i != tag_end && isspace(*i)) ++i;
			// check for parse error (values must be quoted)
			if (i == tag_end || (*i != '\'' && *i != '\"'))
			{
				return false;
			}
			char quote = *i;
			++i;
			val_start = i;
			for (; i != tag_end && *i != quote; ++i);
			// parse error (missing end quote)
			if (i == tag_end)
			{
				return false;
			}

			if (!currentTreeStack.empty())
			{
				elements.push_back({ { start, name_len }, Element::Type::Attribute, true });
				elements.push_back({ { val_start, std::size_t(i - val_start) }, Element::Type::String });

				elements[currentTreeStack.back()].attributesCount++;
			}
		}
	}

	return !elements.empty() && currentTreeStack.empty();
}

std::string_view mtt::XmlParser::Element::value() const
{
	auto valuePtr = this + attributesCount + 1;

	return (hasValue && valuePtr->type == Element::Type::String) || valuePtr->type == Element::Type::Cdata ? valuePtr->name : "";
}

uint64_t mtt::XmlParser::Element::valueNumber(std::string_view name) const
{
	auto v = value(name);
	return v.empty() ? 0 : strtoull(v.data(), nullptr, 10);
}

uint64_t mtt::XmlParser::Element::valueNumber() const
{
	auto v = value();
	return v.empty() ? 0 : strtoull(v.data(), nullptr, 10);
}

std::string_view mtt::XmlParser::Element::value(std::string_view name) const
{
	if (auto node = firstNode(name))
		return node->value();

	return "";
}

const mtt::XmlParser::Element* mtt::XmlParser::Element::attributes() const
{
	return attributesCount ? this + 1 : nullptr;
}

const mtt::XmlParser::Element* mtt::XmlParser::Element::nextSibling() const
{
	return nextSiblingOffset ? this + nextSiblingOffset : nullptr;
}


const mtt::XmlParser::Element* mtt::XmlParser::Element::nextSibling(std::string_view name) const
{
	auto node = nextSibling();

	while (node)
	{
		if (node->name == name)
			break;

		node = node->nextSibling();
	}

	return node;
}

const mtt::XmlParser::Element* mtt::XmlParser::Element::firstNode() const
{
	return firstChildOffset ? this + firstChildOffset : nullptr;
}

const mtt::XmlParser::Element* mtt::XmlParser::Element::firstNode(std::string_view name) const
{
	auto node = firstNode();

	while (node)
	{
		if (node->name == name)
			break;

		node = node->nextSibling();
	}

	return node;
}

mtt::XmlWriter::Element::Element(std::string& b, std::string_view n) : name(n), buffer(b)
{
	buffer += '<';
	buffer += name;
	buffer += '>';
}

mtt::XmlWriter::Element::Element(Element& parent, std::string_view n) : depth(parent.depth + 1), name(n), buffer(parent.buffer)
{
	buffer += '\n';

	for (size_t i = 0; i < depth; i++)
		buffer += '\t';

	buffer += '<';
	buffer += name;
	buffer += '>';

	parent.hasChildren = true;
}

mtt::XmlWriter::Element::Element(Element& parent, std::string_view n, std::initializer_list<std::pair<std::string_view, std::string_view>> attributes) : Element(parent, n)
{
	buffer.pop_back();
	for (auto& a : attributes)
	{
		buffer += ' ';
		buffer += a.first;
		buffer += '=';
		buffer += '"';
		buffer += a.second;
		buffer += '"';
	}
	buffer += '>';
}

mtt::XmlWriter::Element mtt::XmlWriter::Element::createChild(std::string_view n)
{
	return Element(*this, n);
}

void mtt::XmlWriter::Element::addValue(std::string_view n, std::string_view value)
{
	auto c = createChild(n);
	c.setValue(value);
	c.close();
}

void mtt::XmlWriter::Element::addValue(std::string_view n, uint64_t value)
{
	addValue(n, std::to_string(value));
}

mtt::XmlWriter::Element& mtt::XmlWriter::Element::setValue(std::string_view value)
{
	buffer += value;
	return *this;
}

void mtt::XmlWriter::Element::addValueCData(std::string_view n, std::string_view value)
{
	auto c = createChild(n);
	buffer += "<![CDATA[";
	c.setValue(value);
	buffer += "]]>";
	c.close();
}

void mtt::XmlWriter::Element::close()
{
	if (hasChildren)
	{
		buffer += '\n';

		for (size_t i = 0; i < depth; i++)
			buffer += '\t';
	}

	buffer += '<';
	buffer += '/';
	buffer += name;
	buffer += '>';
}

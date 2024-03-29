#include "BencodeParser.h"
#include "Core/Logging.h"
#include <algorithm>
#include <cstring>

#define PARSER_LOG(x) WRITE_GLOBAL_LOG(BencodeParser, x)

template <>
std::string mtt::BencodeParser::Object::getValueOr(const char* name, std::string def) const
{
	auto item = getTxtItem(name);
	return item ? item->toString() : def;
}

bool mtt::BencodeParser::parse(const uint8_t* data, std::size_t length)
{
	deep = 0;
	objects.clear();

	uint32_t reserveSize = length < 200 ? uint32_t(length*0.2f) : std::min(uint32_t(100), uint32_t(40 + length/500));
	objects.reserve(reserveSize);

	bodyEnd = (const char*)data + length;
	bodyStart = (const char*)data;
	auto parserEnd = bodyStart;

	internalParse(&parserEnd);

	remainingData = parserEnd >= bodyEnd ? 0 : (length - (parserEnd - (const char*)data));
	bodyEnd = parserEnd;

	return getRoot() != nullptr;
}

inline bool IS_NUM_CHAR(char c) { return ((c >= '0') && (c <= '9')); }

mtt::BencodeParser::Object* mtt::BencodeParser::internalParse(const char** body)
{
	Object* obj = nullptr;
	char c = **body;

	if (IS_NUM_CHAR(c))
	{
		obj = parseString(body);
	}
	else if (c == 'i')
	{
		obj = parseInt(body);
	}
	else if (c == 'l')
	{
		obj = parseList(body);
	}
	else if (c == 'd')
	{
		obj = parseDictionary(body);
	}
	else
		parseError(body);

	return obj;
}

void mtt::BencodeParser::parseError(const char** body)
{
	PARSER_LOG("PARSE ERROR character " << **body);
	*body = bodyEnd;
}

#define NOT_END_OF_ELEMENTS (**body != 'e' && (*body < bodyEnd))

mtt::BencodeParser::Object* mtt::BencodeParser::parseList(const char** body)
{
	PARSER_LOG("List start " << deep);

	(*body)++;
	deep++;

	int rootId = (int)objects.size();
	objects.emplace_back(mtt::BencodeParser::Object());
	objects.back().info.type = Object::Item::List;

	int count = 0;
	while (NOT_END_OF_ELEMENTS)
	{
		auto objPos = (uint16_t)objects.size();
		auto o = internalParse(body);

		if (o)
		{
			o->info.nextSiblingOffset = NOT_END_OF_ELEMENTS ? (uint16_t)objects.size() - objPos : 0;
			count++;
		}
	}

	(*body)++;
	deep--;

	objects[rootId].info.size = count;
	objects[rootId].info.nextSiblingOffset = NOT_END_OF_ELEMENTS ? (uint16_t)objects.size() - rootId : 0;

	PARSER_LOG("List end " << deep);

	return &objects[rootId];
}

mtt::BencodeParser::Object* mtt::BencodeParser::parseDictionary(const char** body)
{
	PARSER_LOG("Dictionary start" << deep);

	(*body)++;
	deep++;

	int rootId = (int)objects.size();
	objects.emplace_back(mtt::BencodeParser::Object());
	objects.back().info.type = Object::Item::Dictionary;

	int count = 0;
	while (NOT_END_OF_ELEMENTS)
	{
		auto s = parseString(body);

		if (s)
		{
			auto objPos = (uint16_t)objects.size();
			auto o = internalParse(body);

			if (o)
			{
				objects[objPos - 1].info.nextSiblingOffset = 1;
				o->info.nextSiblingOffset = NOT_END_OF_ELEMENTS ? (uint16_t)objects.size() - objPos : 0;
				count++;
			}
			else
				count = 0;
		}
	}

	(*body)++;
	deep--;

	objects[rootId].info.size = count;
	objects[rootId].info.nextSiblingOffset = NOT_END_OF_ELEMENTS ? (uint16_t)objects.size() - rootId : 0;

	PARSER_LOG("Dictionary end" << deep);

	return &objects[rootId];
}

mtt::BencodeParser::Object* mtt::BencodeParser::parseString(const char** body)
{
	mtt::BencodeParser::Object obj;
	char* endPtr = nullptr;
	obj.info.size = strtol(*body, &endPtr, 10);
	*body = endPtr;

	if (**body != ':' || (bodyEnd - *body) < obj.info.size)
	{
		parseError(body);
		return nullptr;
	}

	(*body)++;

	obj.info.type = Object::Item::Text;
	obj.info.data = *body;
	(*body) += obj.info.size;

	PARSER_LOG("String " << obj.info.toString());

	objects.push_back(obj);
	return &objects.back();
}

mtt::BencodeParser::Object* mtt::BencodeParser::parseInt(const char** body)
{
	(*body)++;

	mtt::BencodeParser::Object obj;
	obj.info.data = *body;

	while ((IS_NUM_CHAR(**body) || (**body == '-' && obj.info.size == 0)) && *body != bodyEnd)
	{
		obj.info.size++;
		(*body)++;
	}

	if (**body != 'e')
	{
		parseError(body);
		return nullptr;
	}

	(*body)++;

	obj.info.type = Object::Item::Number;
	PARSER_LOG("Number " << obj.info.toString());

	objects.push_back(obj);
	return &objects.back();
}

mtt::BencodeParser::Object* mtt::BencodeParser::getRoot()
{
	return objects.empty() ? nullptr : &objects[0];
}

const mtt::BencodeParser::Object* mtt::BencodeParser::Object::getDictObject(const char* name) const
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	while (obj)
	{
		if (obj->info.equals(name, len) && (obj + 1)->isMap())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

const mtt::BencodeParser::Object* mtt::BencodeParser::Object::getListObject(const char* name) const
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	while (obj)
	{
		if (obj->info.equals(name, len) && (obj + 1)->isList())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

const mtt::BencodeParser::Object::Item* mtt::BencodeParser::Object::getTxtItem(const char* name) const
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	while(obj)
	{
		if (obj->info.equals(name, len) && (obj + 1)->isText())
			return &(obj + 1)->info;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

const mtt::BencodeParser::Object::Item* mtt::BencodeParser::Object::popTxtItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	while (obj)
	{
		if (obj->info.size && obj->info.equals(name, len) && (obj + 1)->isText())
		{
			const_cast<mtt::BencodeParser::Object*>(obj)->info.size = 0;
			return &(obj + 1)->info;
		}

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

std::string mtt::BencodeParser::Object::getTxt(const char* name) const
{
	auto item = getTxtItem(name);

	return item ? item->toString() : std::string();
}

std::string mtt::BencodeParser::Object::getTxt() const
{
	return isText() ? info.toString() : std::string();
}

const mtt::BencodeParser::Object* mtt::BencodeParser::Object::getIntObject(const char* name) const
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	while (obj)
	{
		if (obj->info.equals(name, len) && (obj + 1)->isInt())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

int mtt::BencodeParser::Object::getInt(const char* name) const
{
	auto o = getIntObject(name);
	return o ? o->getInt() : 0;
}

int mtt::BencodeParser::Object::getInt() const
{
	return strtol(info.data, nullptr, 10);
}

uint64_t mtt::BencodeParser::Object::getBigInt(const char* name) const
{
	auto o = getIntObject(name);
	return o ? o->getBigInt() : 0;
}

uint64_t mtt::BencodeParser::Object::getBigInt() const
{
	return strtoull(info.data, nullptr, 10);
}

const mtt::BencodeParser::Object* mtt::BencodeParser::Object::getFirstItem() const
{
	return info.size ? this + 1 : nullptr;
}

const mtt::BencodeParser::Object* mtt::BencodeParser::Object::getNextSibling() const
{
	return info.nextSiblingOffset ? this + info.nextSiblingOffset : nullptr;
}

bool mtt::BencodeParser::Object::isLastItem() const
{
	return info.nextSiblingOffset == 0;
}

bool mtt::BencodeParser::Object::Item::equals(const char* txt, std::size_t l) const
{
	return l == size && strncmp(txt, data, size) == 0;
}

std::string mtt::BencodeParser::Object::Item::toString() const
{
	return std::string(data, size);
}

bool mtt::BencodeParser::Object::isMap() const
{
	return info.type == Item::Dictionary;
}

bool mtt::BencodeParser::Object::isList() const
{
	return info.type == Item::List;
}

bool mtt::BencodeParser::Object::isInt() const
{
	return info.type == Item::Number;
}

bool mtt::BencodeParser::Object::isText() const
{
	return info.type == Item::Text;
}

const mtt::BencodeParser::Object& mtt::BencodeParser::Object::getDictItemValue() const
{
	return *getNextSibling();
}

bool mtt::BencodeParser::Object::isText(const char* str, std::size_t l) const
{
	return isText() && info.equals(str, l);
}

#include "BencodeParser.h"
#include <algorithm>
#include "..\Core\Logging.h"

#define PARSER_LOG(x) //WRITE_LOG("PARSER LITE: " << x)

bool mtt::BencodeParser::parse(const uint8_t* data, size_t length)
{
	deep = 0;
	objects.clear();

	uint32_t reserveSize = length < 200 ? uint32_t(length*0.2f) : std::min(uint32_t(2000), uint32_t(40 + length/500));
	objects.reserve(reserveSize);

	auto parserEnd = (const char*)data;
	bodyEnd = (const char*)data + length;

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
	char* endPtr = 0;
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

	PARSER_LOG("String " << std::string(obj.data.text.data, obj.data.text.length));

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
	PARSER_LOG("Number " << std::string(obj.info.data, obj.info.size));

	objects.push_back(obj);
	return &objects.back();
}

mtt::BencodeParser::Object* mtt::BencodeParser::getRoot()
{
	return objects.empty() ? nullptr : &objects[0];
}

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getNextSibling()
{
	return info.nextSiblingOffset ? this + info.nextSiblingOffset : nullptr;
}

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getDictItem(const char* name)
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

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getListItem(const char* name)
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

mtt::BencodeParser::Object::Item* mtt::BencodeParser::Object::getTxtItem(const char* name)
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

std::string mtt::BencodeParser::Object::getTxt(const char* name)
{
	auto item = getTxtItem(name);

	return item ? std::string(item->data, item->size) : std::string();
}

std::string mtt::BencodeParser::Object::getTxt()
{
	return isText() ? std::string(info.data, info.size) : std::string();
}

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getIntItem(const char* name)
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

int mtt::BencodeParser::Object::getInt(const char* name)
{
	auto o = getIntItem(name);
	return o ? o->getInt() : 0;
}

int mtt::BencodeParser::Object::getInt()
{
	return strtol(info.data, 0, 10);
}

mtt::BencodeParser::Object* mtt::BencodeParser::Object::getFirstItem()
{
	return info.size ? this + 1 : nullptr;
}

bool mtt::BencodeParser::Object::Item::equals(const char* txt, size_t l)
{
	return l != size ? false : strncmp(txt, data, size) == 0;
}

bool mtt::BencodeParser::Object::isMap()
{
	return info.type == Item::Dictionary;
}

bool mtt::BencodeParser::Object::isList()
{
	return info.type == Item::List;
}

bool mtt::BencodeParser::Object::isInt()
{
	return info.type == Item::Number;
}

bool mtt::BencodeParser::Object::isText()
{
	return info.type == Item::Text;
}

bool mtt::BencodeParser::Object::isText(const char* str, size_t l)
{
	return isText() && info.equals(str, l);
}

#include "BencodeParserLite.h"
#include "..\Core\Logging.h"

#define PARSER_LOG(x) //WRITE_LOG("PARSER LITE: " << x)

mtt::BencodeParserLite::~BencodeParserLite()
{

}

bool mtt::BencodeParserLite::parse(const uint8_t* data, size_t length)
{
	objects.reserve(32);
	auto body = (const char*)data;
	bodyEnd = (const char*)data + length;

	return internalParse(&body) != nullptr;
}

inline bool IS_NUM_CHAR(char c) { return ((c >= '0') && (c <= '9')); }

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::internalParse(const char** body)
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

void mtt::BencodeParserLite::parseError(const char** body)
{
	PARSER_LOG("PARSE ERROR character " << **body);
	*body = bodyEnd;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseList(const char** body)
{
	(*body)++;

	PARSER_LOG("List start " << deep);

	deep++;

	int rootId = (int)objects.size();
	objects.emplace_back(mtt::BencodeParserLite::Object());
	objects.back().info.type = Object::Item::List;

	int count = 0;
	while (**body != 'e' && (*body < bodyEnd))
	{
		auto objPos = (uint16_t)objects.size();
		auto o = internalParse(body);

		if (o)
		{
			o->info.nextSiblingOffset = (uint16_t)objects.size() - objPos;
			count++;
		}
	}

	objects[rootId].info.size = count;

	(*body)++;

	deep--;

	PARSER_LOG("List end " << deep);

	return &objects[rootId];
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseDictionary(const char** body)
{
	(*body)++;

	PARSER_LOG("Dictionary start" << deep);

	deep++;

	int rootId = (int)objects.size();
	objects.emplace_back(mtt::BencodeParserLite::Object());
	objects.back().info.type = Object::Item::Dictionary;

	int count = 0;
	while (**body != 'e' && (*body < bodyEnd))
	{
		auto s = parseString(body);

		if (s)
		{
			auto objPos = (uint16_t)objects.size();
			auto o = internalParse(body);

			if (o)
			{
				o->info.nextSiblingOffset = (uint16_t)objects.size() - objPos;
				count++;
			}
		}
	}

	objects[rootId].info.size = count;

	(*body)++;

	deep--;

	PARSER_LOG("Dictionary end" << deep);

	return &objects[rootId];
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseString(const char** body)
{
	mtt::BencodeParserLite::Object obj;
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

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseInt(const char** body)
{
	(*body)++;

	mtt::BencodeParserLite::Object obj;
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

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::getRoot()
{
	return objects.empty() ? nullptr : &objects[0];
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getNextSibling()
{
	return info.nextSiblingOffset ? this + info.nextSiblingOffset : nullptr;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getDictItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < info.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isMap())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getListItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < info.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isList())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

mtt::BencodeParserLite::Object::Item* mtt::BencodeParserLite::Object::getTxtItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < info.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isText())
			return &(obj + 1)->info;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getNumItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < info.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isInt())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

int mtt::BencodeParserLite::Object::getInt(const char* name)
{
	auto o = getNumItem(name);
	return o ? o->getInt() : 0;
}

int mtt::BencodeParserLite::Object::getInt()
{
	return strtol(info.data, 0, 10);
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getFirstItem()
{
	return this + 1;
}

bool mtt::BencodeParserLite::Object::equals(const char* txt, size_t l)
{
	return l != info.size ? false : strncmp(txt, info.data, info.size) == 0;
}

bool mtt::BencodeParserLite::Object::isMap()
{
	return info.type == Item::Dictionary;
}

bool mtt::BencodeParserLite::Object::isList()
{
	return info.type == Item::List;
}

bool mtt::BencodeParserLite::Object::isInt()
{
	return info.type == Item::Number;
}

bool mtt::BencodeParserLite::Object::isText()
{
	return info.type == Item::Text;
}
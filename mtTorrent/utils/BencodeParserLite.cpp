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
	objects.back().type = Object::Type_List;
	objects.back().data.list.size = 0;

	int count = 0;
	while (**body != 'e' && (*body < bodyEnd))
	{
		int objPos = (int)objects.size();
		auto o = internalParse(body);

		if (o)
		{
			o->nextSiblingOffset = (int)objects.size() - objPos;
			count++;
		}
	}

	objects[rootId].data.list.size = count;

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
	objects.back().type = Object::Type_Dictionary;
	objects.back().data.list.size = 0;

	int count = 0;
	while (**body != 'e' && (*body < bodyEnd))
	{
		auto s = parseString(body);

		if (s)
		{
			int objPos = (int)objects.size();
			auto o = internalParse(body);

			if (o)
			{
				o->nextSiblingOffset = (int)objects.size() - objPos;
				count++;
			}
		}
	}

	objects[rootId].data.map.size = count;

	(*body)++;

	deep--;

	PARSER_LOG("Dictionary end" << deep);

	return &objects[rootId];
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseString(const char** body)
{
	mtt::BencodeParserLite::Object obj;
	char* endPtr = 0;
	obj.data.text.length = strtol(*body, &endPtr, 10);
	*body = endPtr;

	if (**body != ':' || (bodyEnd - *body) < obj.data.text.length)
	{
		parseError(body);
		return nullptr;
	}

	(*body)++;

	obj.type = Object::Type_Text;
	obj.data.text.data = *body;
	(*body) += obj.data.text.length;

	PARSER_LOG("String " << std::string(obj.data.text.data, obj.data.text.length));

	objects.push_back(obj);
	return &objects.back();
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::parseInt(const char** body)
{
	(*body)++;

	mtt::BencodeParserLite::Object obj;
	obj.data.number.length = 0;
	obj.data.number.data = *body;

	while ((IS_NUM_CHAR(**body) || (**body == '-' && obj.data.number.length == 0)) && *body != bodyEnd)
	{
		obj.data.number.length++;
		(*body)++;
	}

	if (**body != 'e')
	{
		parseError(body);
		return nullptr;
	}

	(*body)++;

	obj.type = Object::Type_Number;
	PARSER_LOG("Number " << std::string(obj.data.number.data, obj.data.number.length));

	objects.push_back(obj);
	return &objects.back();
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::getRoot()
{
	return objects.empty() ? nullptr : &objects[0];
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getNextSibling()
{
	return nextSiblingOffset ? this + nextSiblingOffset : nullptr;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getDictItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < data.map.size; i++)
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

	for (int i = 0; i < data.map.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isList())
			return obj + 1;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

mtt::BencodeParserLite::Object::Text* mtt::BencodeParserLite::Object::getTxtItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < data.map.size; i++)
	{
		if (obj->equals(name, len) && (obj + 1)->isText())
			return &(obj + 1)->data.text;

		obj = (obj + 1)->getNextSibling();
	}

	return nullptr;
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getNumItem(const char* name)
{
	auto obj = getFirstItem();
	auto len = strlen(name);

	for (int i = 0; i < data.map.size; i++)
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
	return strtol(data.number.data, 0, 10);
}

mtt::BencodeParserLite::Object* mtt::BencodeParserLite::Object::getFirstItem()
{
	return this + 1;
}

bool mtt::BencodeParserLite::Object::equals(const char* txt, size_t l)
{
	return l != data.text.length ? false : strncmp(txt, data.text.data, data.text.length) == 0;
}

bool mtt::BencodeParserLite::Object::isMap()
{
	return type == Type_Dictionary;
}

bool mtt::BencodeParserLite::Object::isList()
{
	return type == Type_List;
}

bool mtt::BencodeParserLite::Object::isInt()
{
	return type == Type_Number;
}

bool mtt::BencodeParserLite::Object::isText()
{
	return type == Type_Text;
}
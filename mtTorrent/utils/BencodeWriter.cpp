#include "BencodeWriter.h"
#include <cstring>

mtt::BencodeWriter::BencodeWriter(DataBuffer&& buffer)
{
	data = std::move(buffer);
}

void mtt::BencodeWriter::startMap()
{
	data.push_back('d');
}

void mtt::BencodeWriter::startMapItem(const char* name)
{
	addText(name);
	startMap();
}

static void appendText(DataBuffer& data, const char* text, size_t size)
{
	data.insert(data.end(), text, text + size);
}

static void appendText(DataBuffer& data, const char* text)
{
	appendText(data, text, strlen(text));
}

static void appendText(DataBuffer& data, const std::string& text)
{
	appendText(data, text.c_str(), text.length());
}

void mtt::BencodeWriter::startRawMapItem(const char* text)
{
	appendText(data, text);
	startMap();
}

void mtt::BencodeWriter::endMap()
{
	data.push_back('e');
}

void mtt::BencodeWriter::startArray()
{
	data.push_back('l');
}

void mtt::BencodeWriter::startRawArrayItem(const char* text)
{
	appendText(data, text);
	startArray();
}

void mtt::BencodeWriter::endArray()
{
	data.push_back('e');
}

void mtt::BencodeWriter::addItem(const char* name, uint64_t number)
{
	addText(name);
	addNumber(number);
}

void mtt::BencodeWriter::addItem(const char* name, const std::string& text)
{
	addText(name);
	addText(text);
}

void mtt::BencodeWriter::addItem(const char* name, const char* text)
{
	addText(name);
	addText(text);
}

void mtt::BencodeWriter::addItemFromBuffer(const char* name, const char* buffer, std::size_t size)
{
	addText(name);
	appendText(data, std::to_string(size));
	data.push_back(':');
	appendText(data, buffer, size);
}

void mtt::BencodeWriter::addRawItem(const char* name, uint64_t number)
{
	appendText(data, name);
	addNumber(number);
}

void mtt::BencodeWriter::addRawItem(const char* name, const std::string& text)
{
	appendText(data, name);
	addText(text);
}

void mtt::BencodeWriter::addRawItem(const char* name, const char* text)
{
	appendText(data, name);
	addText(text);
}

void mtt::BencodeWriter::addRawItemFromBuffer(const char* name, const char* buffer, std::size_t size)
{
	appendText(data, name);
	appendText(data, std::to_string(size));
	data.push_back(':');
	appendText(data, buffer, size);
}

void mtt::BencodeWriter::addRawData(const uint8_t* buffer, std::size_t size)
{
	data.insert(data.end(), buffer, buffer + size);
}

void mtt::BencodeWriter::addRawData(const char* text)
{
	appendText(data, text);
}

void mtt::BencodeWriter::addNumber(uint64_t number)
{
	data.push_back('i');
	appendText(data, std::to_string(number));
	data.push_back('e');
}

void mtt::BencodeWriter::addText(const char* text)
{
	auto size = strlen(text);
	appendText(data, std::to_string(size));
	data.push_back(':');
	appendText(data, text, size);
}

void mtt::BencodeWriter::addText(const std::string& text)
{
	appendText(data, std::to_string(text.length()));
	data.push_back(':');
	appendText(data, text);
}

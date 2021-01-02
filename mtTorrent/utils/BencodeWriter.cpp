#include "BencodeWriter.h"
#include <cstring>

void mtt::BencodeWriter::startMap()
{
	data.append(1, 'd');
}

void mtt::BencodeWriter::startMap(const char* name)
{
	addText(name);
	startMap();
}

void mtt::BencodeWriter::startRawMap(const char* text)
{
	data.append(text);
	startMap();
}

void mtt::BencodeWriter::endMap()
{
	data.append(1, 'e');
}

void mtt::BencodeWriter::startArray()
{
	data.append(1, 'l');
}

void mtt::BencodeWriter::startRawArrayItem(const char* text)
{
	data.append(text);
	startArray();
}

void mtt::BencodeWriter::endArray()
{
	data.append(1, 'e');
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

void mtt::BencodeWriter::addItemFromBuffer(const char* name, const char* buffer, size_t size)
{
	addText(name);
	data += std::to_string(size);
	data.append(1, ':');
	data.append(buffer, size);
}

void mtt::BencodeWriter::addRawItem(const char* name, uint64_t number)
{
	data.append(name);
	addNumber(number);
}

void mtt::BencodeWriter::addRawItem(const char* name, const std::string& text)
{
	data.append(name);
	addText(text);
}

void mtt::BencodeWriter::addRawItem(const char* name, const char* text)
{
	data.append(name);
	addText(text);
}

void mtt::BencodeWriter::addRawItemFromBuffer(const char* name, const char* buffer, size_t size)
{
	data.append(name);
	data += std::to_string(size);
	data.append(1, ':');
	data.append(buffer, size);
}

void mtt::BencodeWriter::addNumber(uint64_t number)
{
	data.append(1, 'i');
	data += std::to_string(number);
	data.append(1, 'e');
}

void mtt::BencodeWriter::addText(const char* text)
{
	auto size = strlen(text);
	data += std::to_string(size);
	data.append(1, ':');
	data.append(text, size);
}

void mtt::BencodeWriter::addText(const std::string& text)
{
	data += std::to_string(text.length());
	data.append(1, ':');
	data.append(text);
}

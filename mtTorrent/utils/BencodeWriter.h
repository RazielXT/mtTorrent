#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "DataBuffer.h"

namespace mtt
{
	class BencodeWriter
	{
	public:

		BencodeWriter() = default;
		BencodeWriter(DataBuffer&&);

		void startMap();
		void startMapItem(const char* name);
		void startRawMapItem(const char* text);
		void endMap();

		void startArray();
		void startRawArrayItem(const char* text);
		void endArray();
	
		void addNumber(uint64_t number);
		void addText(const char* text);
		void addText(const std::string& text);

		void addItem(const char* name, uint64_t number);
		void addItem(const char* name, const char* text);
		void addItem(const char* name, const std::string& text);
		void addItemFromBuffer(const char* name, const char* buffer, std::size_t size);

		void addRawItem(const char* name, uint64_t number);
		void addRawItem(const char* name, const char* text);
		void addRawItem(const char* name, const std::string& text);
		void addRawItemFromBuffer(const char* name, const char* buffer, std::size_t size);

		void addRawData(const uint8_t* data, std::size_t size);
		void addRawData(const char* text);

		DataBuffer data;
	};

}

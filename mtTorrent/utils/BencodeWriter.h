#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace mtt
{
	class BencodeWriter
	{
	public:

		void startMap();
		void startMap(const char* name);
		void startRawMap(const char* text);
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
		void addItemFromBuffer(const char* name, const char* buffer, size_t size);

		void addRawItem(const char* name, uint64_t number);
		void addRawItem(const char* name, const char* text);
		void addRawItem(const char* name, const std::string& text);
		void addRawItemFromBuffer(const char* name, const char* buffer, size_t size);

		std::string data;
	};

}

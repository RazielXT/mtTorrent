#pragma once

#include <string>

namespace mtt
{
	class allocator;

	struct string
	{
		string();
		~string();

		char* data = nullptr;
		size_t length = 0;

		string& operator=(const std::string& str);
		string& operator=(const char* str);
		string& operator=(const string& str);

		void assign(const char* str, std::size_t length);
		void append(const string& str);

	private:
		const allocator* const allocatorPtr;
	};
}
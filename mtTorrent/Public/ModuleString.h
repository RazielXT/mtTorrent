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

		void assign(const char* str, size_t length);

	private:
		const allocator* const allocator;
	};
}
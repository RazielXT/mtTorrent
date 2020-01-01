#include "UrlEncoding.h"
#include <sstream>

std::string UrlDecode(const std::string& str)
{
	std::string ret;
	char ch;
	size_t i, ii, len = str.length();

	for (i = 0; i < len; i++) {
		if (str[i] != '%') {
			if (str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		}
		else {
			sscanf_s(str.substr(i + 1, 2).c_str(), "%zx", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

std::string UrlEncode(const uint8_t* data, uint32_t size)
{
	static const char lookup[] = "0123456789ABCDEF";
	std::stringstream e;
	for (size_t i = 0, ix = size; i < ix; i++)
	{
		const uint8_t& c = data[i];
		if ((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) //ABC...XYZ
								  //(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}
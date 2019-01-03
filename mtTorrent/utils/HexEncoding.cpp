#include "HexEncoding.h"
#include <algorithm>
#include <cctype>

static uint8_t fromHexa(char h)
{
	if (h <= '9')
		h = h - '0';
	else if(h <= 'f')
		h = h - 'a' + 10;
	else if (h <= 'F')
		h = h - 'A' + 10;

	return h;
}

bool decodeHexa(const std::string& from, uint8_t* to)
{
	for (auto c : from)
	{
		if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			continue;
		else
			return false;
	}

	for (size_t i = 0; i < 20; i++)
	{
		uint8_t f = fromHexa(from[i * 2]);
		uint8_t s = fromHexa(from[i * 2 + 1]);

		to[i] = (f << 4) + s;
	}

	return true;
}

char const hex_chars[] = "0123456789abcdef";

std::string hexToString(uint8_t* in, size_t size)
{
	std::string out;
	out.resize(size*2);

	int idx = 0;
	for (size_t i = 0; i < size; ++i)
	{
		out[idx++] = hex_chars[std::uint8_t(in[i]) >> 4];
		out[idx++] = hex_chars[std::uint8_t(in[i]) & 0xf];
	}

	return out;
}


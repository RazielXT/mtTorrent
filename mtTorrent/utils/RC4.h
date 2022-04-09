#pragma once

#include <cstdint>

struct RC4
{
	void init(const unsigned char* in, size_t len);

	void decode(unsigned char* out, size_t outlen);
	void encode(unsigned char* out, size_t outlen);

	void skip(size_t sz);

private:

	int xs;
	int ys;
	uint8_t buf[256];
};

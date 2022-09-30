#pragma once

#include <cstddef>
#include <cstdint>

struct RC4
{
	void init(const unsigned char* in, std::size_t len);

	void decode(unsigned char* out, std::size_t outlen);
	void encode(unsigned char* out, std::size_t outlen);

	void skip(size_t sz);

private:

	int xs;
	int ys;
	uint8_t buf[256];
};

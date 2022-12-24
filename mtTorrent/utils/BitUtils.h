#pragma once

#include <cstdint>

namespace mtt
{
	namespace utils
	{
		constexpr uint64_t firstBit(uint64_t number) { uint64_t b = 1; while (!(number & b)) b <<= 1; return b; }
	}
}

#ifdef _WIN32

#include <cstdlib>
#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#define swap64 _byteswap_uint64

#elif __GNUC__

uint16_t swap16(uint16_t i);
uint32_t swap32(uint32_t i);
uint64_t swap64(uint64_t i);

#else

#include <byteswap.h>
#define swap16 bswap_16
#define swap32 bswap_32
#define swap64 bswap_64

#endif

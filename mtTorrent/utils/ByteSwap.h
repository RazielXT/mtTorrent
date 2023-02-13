#pragma once

#ifdef _WIN32

#include <cstdlib>
#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#define swap64 _byteswap_uint64

#elif __GNUC__

#include <cstdint>
uint16_t swap16(uint16_t i);
uint32_t swap32(uint32_t i);
uint64_t swap64(uint64_t i);

#else

#include <byteswap.h>
#define swap16 bswap_16
#define swap32 bswap_32
#define swap64 bswap_64

#endif

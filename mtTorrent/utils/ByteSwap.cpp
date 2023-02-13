#include "ByteSwap.h"

#ifdef __GNUC__
uint16_t swap16(uint16_t i) { return __builtin_bswap16(i); }
uint32_t swap32(uint32_t i) { return __builtin_bswap32(i); }
uint64_t swap64(uint64_t i) { return __builtin_bswap64(i); }
#endif //  __GNUC__

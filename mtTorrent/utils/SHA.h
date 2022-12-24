#pragma once

#include <cstring>

#define SHA_DIGEST_LENGTH 20

void _SHA1(const unsigned char* d, std::size_t n, unsigned char* md);

size_t BufferHash(const unsigned char* d, std::size_t n);
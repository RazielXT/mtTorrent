#pragma once
#include <string>

bool decodeHexa(const std::string& hexString, uint8_t* out);
std::string hexToString(const uint8_t* in, size_t size);
#pragma once

#include <string>

bool decodeHexa(const std::string& hexString, uint8_t* out, size_t size = 20);
std::string hexToString(const uint8_t* in, size_t size);
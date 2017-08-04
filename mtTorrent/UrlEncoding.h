#pragma once
#include <string>

std::string UrlDecode(const std::string& src);

std::string UrlEncode(uint8_t* data, uint32_t size);
#pragma once

#include "Network.h"

struct HttpHeaderInfo
{
	bool valid = true;
	bool success = false;
	uint32_t dataStart = 0;
	uint32_t dataSize = 0;

	std::vector<std::pair<std::string, std::string>> headerParameters;

	static HttpHeaderInfo readFromBuffer(const BufferView& buffer);
	static HttpHeaderInfo read(const char* buffer, std::size_t bufferSize);
};

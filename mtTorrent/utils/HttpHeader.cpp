#include "HttpHeader.h"

struct DataChunk
{
	uint32_t start;
	uint32_t size;
};
std::vector<DataChunk> chunks;

std::vector<DataChunk> readChunkedData(const char* buffer, size_t bufferSize, size_t bufferDataStart)
{
	std::vector<DataChunk> chunks;
	uint32_t chunkSize = 0;

	auto ptr = buffer + bufferDataStart;
	size_t size = bufferSize - bufferDataStart;
	auto bufferEnd = ptr + size;
	while (ptr < bufferEnd)
	{
		size_t pos = 0;
		while (pos + 1 < size)
		{
			if (ptr[pos] == '\r' && ptr[pos + 1] == '\n')
				break;
			pos++;
		}

		if (pos + 1 > size)
			return {};

		if (pos == 0)
			chunkSize = 0;
		else
		{
			if (chunkSize == 0)
				chunkSize = strtoul((const char*)ptr, nullptr, 16);
			else
			{
				chunks.push_back({ (uint32_t)(ptr - buffer), chunkSize });
				chunkSize = 0;
			}
		}

		ptr += pos + 2;
		size -= pos + 2;
	}

	return chunks;
}

HttpHeaderInfo HttpHeaderInfo::readFromBuffer(DataBuffer& buffer)
{
	return HttpHeaderInfo::read((const char*)buffer.data(), buffer.size());
}

HttpHeaderInfo HttpHeaderInfo::read(const char* buffer, size_t bufferSize)
{
	HttpHeaderInfo info;

	if (bufferSize < 4 || !(strncmp(buffer, "HTTP", 4) == 0 || strncmp(buffer, "http", 4) == 0))
	{
		info.valid = false;
		return info;
	}

	size_t pos = 0;
	while (pos + 1 < bufferSize)
	{
		if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
		{
			info.dataStart = (uint32_t)pos + 2;
			break;
		}

		std::string line;
		for (size_t i = pos + 1; i < bufferSize - 1; i++)
		{
			if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			{
				line = std::string((char*)& buffer[pos], (char*)& buffer[i]);
				pos = i + 2;
				break;
			}
		}

		auto vpos = line.find_first_of(':');
		if (vpos != std::string::npos && line.length() > vpos + 1)
		{
			info.headerParameters.push_back({ line.substr(0, vpos),line.substr(vpos + 2) });
		}
		else
			info.headerParameters.push_back({ line,"" });
	}

	for (auto& p : info.headerParameters)
	{
		std::transform(p.first.begin(), p.first.end(), p.first.begin(), ::toupper);
		std::transform(p.second.begin(), p.second.end(), p.second.begin(), ::toupper);

		if (p.first == "CONTENT-LENGTH" && !p.second.empty())
			info.dataSize = std::stoul(p.second);

		if (!info.dataSize && p.first == "TRANSFER-ENCODING" && p.second == "CHUNKED")
		{
			auto chunks = readChunkedData(buffer, bufferSize, info.dataStart);
			if (!chunks.empty())
			{
				info.dataStart = chunks.front().start;
				info.dataSize = chunks.front().size;
			}

		}
	}

	if (info.dataStart && !(info.dataSize || bufferSize == info.dataStart))
		info.valid = false;

	if (!info.headerParameters.empty() && info.headerParameters[0].first.find("200 OK") != std::string::npos)
		info.success = true;

	return info;
}

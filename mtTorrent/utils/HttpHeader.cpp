#include "HttpHeader.h"

HttpHeaderInfo HttpHeaderInfo::readFromBuffer(DataBuffer& buffer)
{
	HttpHeaderInfo info;

	size_t pos = 0;
	while (pos + 1 < buffer.size())
	{
		if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
		{
			info.dataStart = (uint32_t)pos + 2;
			break;
		}

		std::string line;
		for (size_t i = pos + 1; i < buffer.size() - 1; i++)
		{
			if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			{
				line = std::string((char*)& buffer[pos], (char*)& buffer[i]);
				pos = i + 2;
				break;
			}
		}

		if (line.find_first_of(':') != std::string::npos)
		{
			auto vpos = line.find_first_of(':');
			info.headerParameters.push_back({ line.substr(0, vpos),line.substr(vpos + 2) });
		}
		else
			info.headerParameters.push_back({ line,"" });
	}

	for (auto& p : info.headerParameters)
	{
		if ((p.first == "Content-Length" || p.first == "CONTENT-LENGTH") && !p.second.empty())
			info.dataSize = std::stoul(p.second);
	}

	if (info.dataStart && !info.dataSize)
		info.valid = false;

	if (!info.headerParameters.empty() && info.headerParameters[0].first.find("200 OK") != std::string::npos)
		info.success = true;

	return info;
}

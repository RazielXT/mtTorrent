#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "Interface.h"
#include "BencodeParser.h"

namespace mtt
{
	namespace TorrentFileParser
	{
		struct ParsedInfo
		{
			const char* infoStart = nullptr;
			size_t infoSize = 0;
		};

		TorrentFileInfo parse(const uint8_t* data, size_t length, ParsedInfo* info = nullptr);

		TorrentInfo parseTorrentInfo(const uint8_t* data, size_t length);
	}
}

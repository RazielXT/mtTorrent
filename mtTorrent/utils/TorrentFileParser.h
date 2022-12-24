#pragma once

#include "Interface.h"
#include "BencodeParser.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>

namespace mtt
{
	namespace TorrentFileParser
	{
		TorrentFileInfo parse(const uint8_t* data, std::size_t length);

		TorrentInfo parseTorrentInfo(const uint8_t* data, std::size_t length);
	}
}

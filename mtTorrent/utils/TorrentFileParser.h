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
		TorrentFileInfo parse(const uint8_t* data, size_t length);

		TorrentInfo parseTorrentInfo(const uint8_t* data, size_t length);
	}
}

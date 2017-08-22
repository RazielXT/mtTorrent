#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "Interface.h"
#include "BencodeParser.h"

namespace mtt
{
	class TorrentFileParser
	{
	public:

		bool parseFile(const char* filename);
		bool parse(const uint8_t* data, size_t length);

		TorrentFileInfo fileInfo;

		TorrentInfo parseTorrentInfo(const uint8_t* data, size_t length);

	private:

		bool generateInfoHash(BencodeParser& parsed);
		void loadTorrentFileInfo(BencodeParser& parsed);

		TorrentInfo parseTorrentInfo(BencodeParser::Object* info);
	};

}

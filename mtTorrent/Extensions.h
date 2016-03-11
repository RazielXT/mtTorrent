#pragma once
#include "BencodeParser.h"

namespace Torrent
{
	struct PeerExchangeExtension
	{
		std::vector<std::string> contacts;
		std::string contactsMessage;

		bool load(BencodeParser::Object& extensions);
	};
}

#include "Extensions.h"

bool Torrent::PeerExchangeExtension::load(BencodeParser::Object& extHandshake)
{
	if (extHandshake.type == BencodeParser::Object::Dictionary)
	{
		auto& ext = extHandshake.dic->find("m");

		if (ext != extHandshake.dic->end() && ext->second.type == BencodeParser::Object::Dictionary)
		{
			auto& extensions = *ext->second.dic;

			if (extensions.find("ut_pex") != extensions.end())
			{
				auto& pexInfo = extensions["ut_pex"];

				if (pexInfo.type == BencodeParser::Object::Text)
				{
					contactsMessage = pexInfo.txt;
					return true;
				}
			}
		}
	}

	return false;
}


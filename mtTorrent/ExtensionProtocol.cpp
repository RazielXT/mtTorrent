#include "ExtensionProtocol.h"
#include "PacketHelper.h"
#include "PeerMessage.h"

void Torrent::PeerExchangeExtension::load(BencodeParser::Object& data)
{
	added.clear();
	addedFlags.clear();

	if (data.type == BencodeParser::Object::Dictionary)
	{
		auto& add = data.dic->find("added");

		if (add != data.dic->end() && add->second.type == BencodeParser::Object::Text)
		{
			added = add->second.txt;
			size_t pos = 0;

			while (added.size() - pos >= 6)
			{
				uint32_t ip = swap32(*reinterpret_cast<uint32_t*>(&added[0] + pos));

				PeerInfo peer;
				peer.setIp(ip);
				peer.port = swap16(*reinterpret_cast<uint16_t*>(&added[0] + pos + 4));

				addedPeers.push_back(peer);
				pos += 6;
			}
		}

		auto& addF = data.dic->find("added.f");

		if (addF != data.dic->end() && addF->second.type == BencodeParser::Object::Text)
		{
			addedFlags = addF->second.txt;
		}
	}
}

void Torrent::ExtensionProtocol::setInfo(ClientInfo* client)
{
	clientInfo = client;
}

Torrent::ExtendedMessageType Torrent::ExtensionProtocol::load(char id, DataBuffer& data)
{
	if (id >= InvalidEx)
		return InvalidEx;

	parser.parse(data);

	if (id == HandshakeEx)
	{
		if (parser.parsedData.type == BencodeParser::Object::Dictionary)
		{
			auto& ext = parser.parsedData.dic->find("m");

			if (ext != parser.parsedData.dic->end() && ext->second.type == BencodeParser::Object::Dictionary)
			{
				auto& extensions = *ext->second.dic;

				if (extensions.find("ut_pex") != extensions.end())
				{
					auto& pexInfo = extensions["ut_pex"];

					if (pexInfo.type == BencodeParser::Object::Number)
					{
						messageIds[pexInfo.i] = PexEx;
					}
				}
			}
		}

		return HandshakeEx;
	}
	else
	{
		auto idIt = messageIds.find(id);

		if (idIt != messageIds.end())
		{
			auto msgType = messageIds[id];

			if (msgType == PexEx)
			{
				pex.load(parser.parsedData);
			}

			return msgType;
		}
	}

	return InvalidEx;
}

DataBuffer Torrent::ExtensionProtocol::getExtendedHandshakeMessage()
{
	PacketBuilder builder;

	//std::string extDict = "d1:md6:ut_pexi" + std::to_string(PexEx) + "ee";
	std::string extDict = "d1:md11:LT_metadatai2e6:ut_pexi" + std::to_string(PexEx) + "ee1:pi" + std::to_string(clientInfo->listenPort) + "e1:v13:mtTorrent 0.1e";
	builder.add32(static_cast<uint32_t>(2 + extDict.length()));
	builder.add(Extended);
	builder.add(HandshakeEx);
	
	builder.add(extDict.data(), extDict.length());

	return builder.getBuffer();
}

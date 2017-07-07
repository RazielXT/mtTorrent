#include "ExtensionProtocol.h"
#include "PacketHelper.h"
#include "PeerMessage.h"

using namespace mtt::ext;

PeerExchange::Message PeerExchange::load(BencodeParser::Object& data)
{
	Message msg;

	if (data.type == BencodeParser::Object::Dictionary)
	{
		auto& add = data.dic->find("added");

		if (add != data.dic->end() && add->second.type == BencodeParser::Object::Text)
		{
			msg.added = add->second.txt;
			size_t pos = 0;

			while (msg.added.size() - pos >= 6)
			{
				uint32_t ip = swap32(*reinterpret_cast<uint32_t*>(&msg.added[0] + pos));

				PeerInfo peer;
				peer.setIp(ip);
				peer.port = swap16(*reinterpret_cast<uint16_t*>(&msg.added[0] + pos + 4));

				msg.addedPeers.push_back(peer);
				pos += 6;
			}
		}

		auto& addF = data.dic->find("added.f");

		if (addF != data.dic->end() && addF->second.type == BencodeParser::Object::Text)
		{
			msg.addedFlags = addF->second.txt;
		}
	}
}

/*
bool mtt::UtMetadataExtension::isFull()
{
	return size && !remainingPiecesFlag;
}

void mtt::UtMetadataExtension::setSize(int s)
{
	size = s;
	remainingPiecesFlag = 0;

	uint32_t flagPos = 1;
	while (s > 0)
	{
		remainingPiecesFlag |= flagPos;
		flagPos <<= 1;
		s -= 16 * 1024;
	}

	metadata.resize(s);
}
*/

UtMetadata::Message UtMetadata::load(BencodeParser::Object& data, const char* remainingData, size_t remainingSize)
{
	Message msg;
	msg.size = size;

	if (data.isMap())
	{
		if (auto msgType = data.getIntItem("msg_type"))
		{
			msg.id = (MessageId)*msgType;

			if (msg.id == Request)
			{
			}
			else if (msg.id == Data)
			{
				auto piece = data.getIntItem("piece");
				auto piecesize = data.getIntItem("total_size");

				if (piece && piecesize && *piecesize <= remainingSize)
				{
					/*uint32_t flagPos = (uint32_t)(1 << *piece);
					if (remainingPiecesFlag & flagPos)
					{
						remainingPiecesFlag ^= flagPos;
						size_t offset = *piece * 16 * 1024;
						memcpy(&metadata[0] + offset, remainingData, *piecesize);
					}*/

					msg.metadata.insert(msg.metadata.begin(), remainingData, remainingData + *piecesize);
				}
			}
			else if (msg.id == Reject)
			{
			}
		}
	}
}

MessageType ExtensionProtocol::load(char id, DataBuffer& data)
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

				if (extensions.find("ut_metadata") != extensions.end())
				{
					auto& utmInfo = extensions["ut_metadata"];

					if (utmInfo.type == BencodeParser::Object::Number)
					{
						messageIds[utmInfo.i] = UtMetadataEx;
					}

					auto& utmSize = parser.parsedData.dic->find("metadata_size");
					if (utmSize != parser.parsedData.dic->end() && utmSize->second.type == BencodeParser::Object::Number)
					{
						utm.size = utmSize->second.i;
					}
					else
						utm.size = 0;
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
				auto msg = pex.load(parser.parsedData);

				if (onPexMessage)
					onPexMessage(msg);
			}

			if (msgType == UtMetadataEx)
			{
				auto msg = utm.load(parser.parsedData, parser.bodyEnd, parser.remainingData);

				if (onUtMetadataMessage)
					onUtMetadataMessage(msg);
			}

			return msgType;
		}
	}

	return InvalidEx;
}

DataBuffer ExtensionProtocol::getExtendedHandshakeMessage(bool enablePex, uint16_t metadataSize)
{
	PacketBuilder extDict(100);
	extDict.add32(0);
	extDict.add(mtt::Extended);
	extDict.add(HandshakeEx);

	//std::string extDict = "d1:md11:LT_metadatai2e6:ut_pexi" + std::to_string(PexEx) + "ee1:pi" + std::to_string(mtt::getClientInfo()->listenPort) + "e1:v" + std::to_string(strlen(MT_NAME)) +":" + MT_NAME + "e";
	
	extDict.add('d');

	if (enablePex || metadataSize)
	{
		extDict.add("1:md", 4);

		if (metadataSize)
		{
			extDict.add("ut_metadatai", 12);
			extDict.add((char)UtMetadataEx);
			extDict.add('e');
		}

		if (enablePex)
		{
			extDict.add("6:ut_pexi", 9);
			extDict.add((char)PexEx);
			extDict.add('e');
		}

		extDict.add('e');
	}

	if (metadataSize)
	{
		extDict.add("13:metadata_sizei", 17);
		auto sizeStr = std::to_string(metadataSize);
		extDict.add(sizeStr.data(), sizeStr.length());
		extDict.add('e');
	}

	extDict.add("1:pi", 4);
	auto portStr = std::to_string(mtt::getClientInfo()->listenPort);
	extDict.add(portStr.data(), portStr.length());
	extDict.add('e');

	extDict.add("1:v", 3);
	auto nameLen = std::to_string(_countof(MT_NAME));
	extDict.add(nameLen.data(), nameLen.length());
	extDict.add(':');
	extDict.add(MT_NAME, _countof(MT_NAME));

	extDict.add('e');

	*reinterpret_cast<uint32_t*>(&extDict.out[0]) = swap32((uint32_t)(extDict.out.size() - sizeof(uint32_t)));

	return extDict.getBuffer();
}

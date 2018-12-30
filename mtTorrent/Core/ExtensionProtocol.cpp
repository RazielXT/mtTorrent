#include "ExtensionProtocol.h"
#include "utils/PacketHelper.h"
#include "PeerMessage.h"
#include "Configuration.h"

#define BT_EXT_LOG(x) WRITE_LOG(LogTypeBtExt, x)

using namespace mtt;
using namespace mtt::ext;

void PeerExchange::load(BencodeParser::Object* data)
{
	Message msg;

	if (data && data->isMap())
	{
		if (auto added = data->getTxtItem("added"))
		{
			uint32_t pos = 0;

			while (added->size - pos >= 6)
			{
				uint32_t ip = swap32(*reinterpret_cast<const uint32_t*>(added->data + pos));
				uint16_t port = swap16(*reinterpret_cast<const uint16_t*>(added->data + pos + 4));

				Addr peer;
				peer.set(ip, port);

				msg.addedPeers.push_back(peer);
				pos += 6;
			}
		}

		if (auto addedF = data->getTxtItem("added.f"))
		{
			msg.addedFlags = std::string(addedF->data, addedF->size);
		}
	}

	BT_EXT_LOG("PEX peers count " << msg.addedPeers.size());

	onPexMessage(msg);
}

void UtMetadata::load(BencodeParser::Object* data, const char* remainingData, size_t remainingSize)
{
	if (data && data->isMap())
	{
		if (auto msgType = data->getIntItem("msg_type"))
		{
			Message msg;
			msg.size = size;
			msg.id = (MessageId)msgType->getInt();

			if (msg.id == Request)
			{
			}
			else if (msg.id == Data)
			{
				msg.piece = data->getInt("piece");
				msg.size = data->getInt("total_size");
				
				if (msg.size && remainingSize <= 16*1024)
				{
					msg.metadata.insert(msg.metadata.begin(), remainingData, remainingData + remainingSize);
				}

				BT_EXT_LOG("UTM data piece id " << msg.piece << " size/total: " << remainingSize << "/" << msg.size);

				onUtMetadataMessage(msg);
			}
			else if (msg.id == Reject)
			{
			}
		}
	}
}

DataBuffer mtt::ext::UtMetadata::createMetadataRequest(uint32_t index)
{
	PacketBuilder packet(32);
	packet.add32(0);
	packet.add(mtt::Extended);
	packet.add(UtMetadataEx);
	packet.add("d8:msg_typei0e5:piecei", 22);
	auto idxStr = std::to_string(index);
	packet.add(idxStr.data(), idxStr.length());
	packet.add("ee", 2);

	*reinterpret_cast<uint32_t*>(&packet.out[0]) = swap32((uint32_t)(packet.out.size() - sizeof(uint32_t)));

	return packet.getBuffer();
}

MessageType ExtensionProtocol::load(char id, DataBuffer& data)
{
	if (id >= InvalidEx)
		return InvalidEx;

	BencodeParser parser;
	if (!parser.parse(data.data(), data.size()))
		return InvalidEx;

	auto root = parser.getRoot();

	if (id == HandshakeEx)
	{
		if (root->isMap())
		{
			if (auto extensionsInfo = root->getDictItem("m"))
			{
				if (auto pexId = extensionsInfo->getIntItem("ut_pex"))
				{
					messageIds[pexId->getInt()] = PexEx;
				}

				if (auto utmId = extensionsInfo->getIntItem("ut_metadata"))
				{
					messageIds[utmId->getInt()] = UtMetadataEx;
					utm.size = root->getInt("metadata_size");
				}
			}

			if (auto version = root->getTxtItem("v"))
				state.client = std::string(version->data, version->size);

			if (auto ip = root->getTxtItem("yourip"))
				state.yourIp = std::string(ip->data, ip->size);

			state.enabled = true;

			if (!state.sentHandshake)
				sendHandshake();
		}

		return HandshakeEx;
	}
	else
	{
		auto idType = messageIds.find(id);

		if (idType != messageIds.end())
		{
			auto msgType = idType->second;

			if (msgType == PexEx)
				pex.load(parser.getRoot());
			else if (msgType == UtMetadataEx)
				utm.load(parser.getRoot(), parser.bodyEnd, parser.remainingData);

			return msgType;
		}
	}

	return InvalidEx;
}

DataBuffer ExtensionProtocol::createExtendedHandshakeMessage(bool enablePex, uint16_t metadataSize)
{
	PacketBuilder extDict(100);
	extDict.add32(0);
	extDict.add(mtt::Extended);
	extDict.add(HandshakeEx);

	extDict.add('d');

	if (enablePex || !metadataSize)
	{
		extDict.add("1:md", 4);

		extDict.add("11:ut_metadatai", 15);
		extDict.add('0' + UtMetadataEx);
		extDict.add('e');

		if (enablePex)
		{
			extDict.add("6:ut_pexi", 9);
			extDict.add('0' + PexEx);
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
	auto portStr = std::to_string(mtt::config::external.tcpPort);
	extDict.add(portStr.data(), portStr.length());
	extDict.add('e');

	extDict.add("1:v", 3);
	auto nameLen = std::to_string(13);
	extDict.add(nameLen.data(), nameLen.length());
	extDict.add(':');
	extDict.add(MT_NAME, 13);

	extDict.add('e');

	*reinterpret_cast<uint32_t*>(&extDict.out[0]) = swap32((uint32_t)(extDict.out.size() - sizeof(uint32_t)));

	return extDict.getBuffer();
}

bool mtt::ext::ExtensionProtocol::isSupported(MessageType type)
{
	for (auto&m : messageIds)
		if (m.second == type)
			return true;

	return false;
}

void mtt::ext::ExtensionProtocol::sendHandshake()
{
	if(!state.sentHandshake)
		stream->write(createExtendedHandshakeMessage());

	state.sentHandshake = true;
}

bool mtt::ext::ExtensionProtocol::requestMetadataPiece(uint32_t index)
{
	if (!state.enabled || utm.size == 0)
		return false;

	stream->write(utm.createMetadataRequest(index));

	return true;
}

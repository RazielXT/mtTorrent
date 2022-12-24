#include "MetadataExtension.h"
#include "utils/PacketHelper.h"
#include "utils/BencodeParser.h"
#include "utils/BencodeWriter.h"
#include "PeerCommunication.h"

#define BT_EXT_LOG(x) WRITE_GLOBAL_LOG(ExtensionProtocol, x)

using namespace mtt;
using namespace mtt::ext;

bool UtMetadata::Load(const BufferView& data, Message& msg)
{
	BencodeParser parser;
	if (!parser.parse(data.data, data.size))
		return false;

	auto root = parser.getRoot();
	if (!root || !root->isMap())
		return false;

	if (auto msgType = root->getIntObject("msg_type"))
	{
		msg.type = (Type)msgType->getInt();

		if (msg.type == Type::Request)
		{
			msg.piece = root->getInt("piece");
		}
		else if (msg.type == Type::Data)
		{
			msg.piece = root->getInt("piece");
			msg.totalSize = root->getInt("total_size");

			if (msg.totalSize && parser.remainingData <= PieceSize)
			{
				msg.metadata = { parser.bodyEnd, parser.remainingData };
			}

			BT_EXT_LOG("UTM data piece id " << msg.piece << " size/total: " << parser.remainingData << "/" << msg.totalSize);
		}
		else if (msg.type == Type::Reject)
		{
			msg.piece = root->getInt("piece");
		}
		else
			return false;
	}

	return true;
}

static void setBufferSizeHeader(DataBuffer& data)
{
	PacketBuilder::Assign32(data.data(), (uint32_t)(data.size() - sizeof(uint32_t)));
}

void UtMetadata::sendPieceRequest(uint32_t index)
{
	BencodeWriter writer;
	writer.data.reserve(32);

	writer.data.resize(sizeof(uint32_t)); //length
	writer.data.push_back(mtt::PeerMessage::Extended);
	writer.data.push_back(messageId);

	writer.startMap();
	writer.addRawItem("8:msg_type", (uint64_t)Type::Request);
	writer.addRawItem("5:piece", index);
	writer.endMap();

	setBufferSizeHeader(writer.data);

	peer->send(writer.data);
}

void UtMetadata::sendPieceResponse(uint32_t index, BufferView data, size_t totalSize)
{
	BencodeWriter writer;
	writer.data.reserve(data.size + 55);

	writer.data.resize(sizeof(uint32_t)); //length
	writer.data.push_back(mtt::PeerMessage::Extended);
	writer.data.push_back(messageId);

	writer.startMap();
	writer.addRawItem("8:msg_type", (uint32_t)Type::Data);
	writer.addRawItem("5:piece", index);
	writer.addRawItem("10:total_size", totalSize);
	writer.endMap();

	writer.addRawData(data.data, data.size);

	setBufferSizeHeader(writer.data);

	peer->send(writer.data);
}

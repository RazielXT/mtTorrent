#include "PeerMessage.h"
#include "PacketHelper.h"

using namespace Torrent;

PeerMessage PeerMessage::loadMessage(std::vector<char>& data)
{
	PeerMessage msg;

	if (data.empty())
		return msg;

	if (data.size() >= 68 && data[0] == 19)
	{
		//auto str = std::string(data.begin() + 1, data.begin() + 20);
		//if (str == "BitTorrent protocol")
		msg.id = Handshake;

		msg.messageSize = 68;
		memcpy(msg.peer_id, &data[0] + 20 + 8 + 20, 20);

		return msg;
	}

	PacketReader reader(data);

	auto size = reader.pop32();
	msg.messageSize = size + 4;

	//incomplete
	if (reader.getRemainingSize() < size)
		return msg;

	if (size == 0)
	{
		msg.id = KeepAlive;
	}
	else
	{
		msg.id = PeerMessageId(reader.pop());

		if (msg.id == Have && size == 5)
		{
			msg.pieceIndex = reader.pop32();
		}
		else if (msg.id == Bitfield)
		{
			msg.bitfield = reader.popBuffer(size - 1);
		}
		else if (msg.id == Request && size == 13)
		{
			msg.request.index = reader.pop32();
			msg.request.begin = reader.pop32();
			msg.request.length = reader.pop32();
		}
		else if (msg.id == Request && size > 9)
		{
			msg.piece.index = reader.pop32();
			msg.piece.begin = reader.pop32();
			msg.piece.block = reader.popBuffer(size - 9);
		}
		else if (msg.id == Cancel && size == 13)
		{
			msg.request.index = reader.pop32();
			msg.request.begin = reader.pop32();
			msg.request.length = reader.pop32();
		}
		else if (msg.id == Port && size == 3)
		{
			msg.port = reader.pop16();
		}
	}

	if (msg.id >= Invalid)
	{
		msg.id = Invalid;
		msg.messageSize = 0;
	}

	return msg;
}
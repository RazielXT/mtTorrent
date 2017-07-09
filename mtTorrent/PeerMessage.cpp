#include "PeerMessage.h"
#include "PacketHelper.h"

using namespace mtt;

PeerMessage::PeerMessage(DataBuffer& data)
{
	if (data.empty())
		return;

	if (data.size() >= 68 && data[0] == 19)
	{
		if (strncmp(data.data() + 1, "BitTorrent protocol", 19) == 0)
		{
			id = Handshake;

			messageSize = 68;
			memcpy(handshake.peerId, &data[0] + 20 + 8 + 20, 20);
			memcpy(handshake.reservedBytes, &data[0] + 20, 8);

			return;
		}
	}

	PacketReader reader(data);

	auto size = reader.pop32();
	messageSize = size + 4;

	//incomplete
	if (reader.getRemainingSize() < size)
		return;

	if (size == 0)
	{
		id = KeepAlive;
	}
	else
	{
		id = PeerMessageId(reader.pop());

		if (id == Have && size == 5)
		{
			havePieceIndex = reader.pop32();
		}
		else if (id == Bitfield)
		{
			bitfield = reader.popBuffer(size - 1);
		}
		else if (id == Request && size == 13)
		{
			request.index = reader.pop32();
			request.begin = reader.pop32();
			request.length = reader.pop32();
		}
		else if (id == Piece && size > 9)
		{
			piece.info.index = reader.pop32();
			piece.info.begin = reader.pop32();
			piece.data = reader.popBuffer(size - 9);
			piece.info.length = static_cast<uint32_t>(piece.data.size());
		}
		else if (id == Cancel && size == 13)
		{
			request.index = reader.pop32();
			request.begin = reader.pop32();
			request.length = reader.pop32();
		}
		else if (id == Port && size == 3)
		{
			port = reader.pop16();
		}
		else if (id == Extended && size > 2)
		{
			extended.id = reader.pop();
			extended.data = reader.popBuffer(size - 2);
		}
	}

	if (id >= Invalid)
	{
		id = Invalid;
		messageSize = 0;
	}
}
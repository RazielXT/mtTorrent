#include "PeerMessage.h"
#include "PacketHelper.h"

using namespace Torrent;

PeerMessage::PeerMessage(std::vector<char>& data)
{
	if (data.empty())
		return;

	if (data.size() >= 68 && data[0] == 19)
	{
		auto str = std::string(data.begin() + 1, data.begin() + 20);
		if (str == "BitTorrent protocol")
		{
			id = Handshake;

			messageSize = 68;
			memcpy(peer_id, &data[0] + 20 + 8 + 20, 20);

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
			piece.index = reader.pop32();
			piece.begin = reader.pop32();
			piece.block = reader.popBuffer(size - 9);
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
	}

	if (id >= Invalid)
	{
		id = Invalid;
		messageSize = 0;
	}
}
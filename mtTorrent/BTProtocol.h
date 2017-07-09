#pragma once

#include "Interface2.h"
#include "PacketHelper.h"

namespace mtt
{
	namespace bt
	{
		DataBuffer createHandshake(uint8_t* torrentHash, uint8_t* clientHash)
		{
			PacketBuilder packet(70);
			packet.add(19);
			packet.add("BitTorrent protocol", 19);

			uint8_t reserved_byte[8] = { 0 };
			reserved_byte[5] |= 0x10;	//Extension Protocol

			packet.add(reserved_byte, 8);

			packet.add(torrentHash, 20);
			packet.add(clientHash, 20);

			return packet.getBuffer();
		}

		DataBuffer createInterested()
		{
			PacketBuilder packet(5);
			packet.add32(1);
			packet.add(Interested);

			return packet.getBuffer();
		}

		DataBuffer createBlockRequest(PieceBlockInfo& block)
		{
			PacketBuilder packet;
			packet.add32(13);
			packet.add(Request);
			packet.add32(block.index);
			packet.add32(block.begin);
			packet.add32(block.length);

			return packet.getBuffer();
		}
	}
}
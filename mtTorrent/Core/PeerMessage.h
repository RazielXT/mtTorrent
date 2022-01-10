#pragma once

#include <vector>
#include "Interface.h"

namespace mtt
{
	struct PeerMessage
	{
		enum Id
		{
			Choke = 0,
			Unchoke,
			Interested,
			NotInterested,
			Have,
			Bitfield,
			Request,
			Piece,
			Cancel,
			Port,
			Extended = 20,
			Handshake,
			KeepAlive,
			Invalid
		};
		Id id = Invalid;

		uint32_t havePieceIndex;
		DataBuffer bitfield;

		struct  
		{
			uint8_t reservedBytes[8];
			uint8_t peerId[20];
			uint8_t info[20];
		}
		handshake;

		PieceBlockInfo request;
		PieceBlock piece;

		uint16_t port;
		uint16_t messageSize = 0;

		PeerMessage(const BufferView& buffer);

		struct
		{
			uint8_t id;
			DataBuffer data;
		}
		extended;	
	};
}

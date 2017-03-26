#pragma once
#include <vector>
#include "TorrentDefines.h"

namespace mtt
{
	enum PeerMessageId
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
		KeepAlive,
		Extended = 20,
		Handshake,
		Invalid
	};

	struct PeerMessage
	{
		PeerMessageId id = Invalid;

		uint32_t havePieceIndex;
		DataBuffer bitfield;

		uint8_t peer_id[20];

		PieceBlockInfo request;
		PieceBlock piece;

		uint16_t port;
		uint16_t messageSize = 0;

		PeerMessage(DataBuffer& data);

		struct
		{
			char id;
			DataBuffer data;
		}
		extended;	
	};
}

#pragma once
#include <vector>
#include "Interface.h"

namespace Torrent
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

	enum PeerExtendedMessageId
	{
		HandshakeEx = 0,
		InvalidEx
	};

	struct PeerMessage
	{
		PeerMessageId id = Invalid;

		uint32_t havePieceIndex;
		std::vector<char> bitfield;

		uint8_t peer_id[20];

		PieceBlockInfo request;
		PieceBlock piece;

		uint16_t port;
		uint16_t messageSize = 0;

		PeerMessage(std::vector<char>& data);

		struct 
		{
			PeerExtendedMessageId id;

			std::vector<char> handshakeExt;
		}
		extended;
	};
}

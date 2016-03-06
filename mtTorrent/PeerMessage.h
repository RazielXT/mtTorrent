#pragma once
#include <vector>

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
		Handshake,
		Invalid
	};

	struct PeerMessage
	{
		PeerMessageId id = Invalid;

		uint32_t pieceIndex;
		std::vector<char> bitfield;

		uint8_t peer_id[20];

		struct
		{
			uint32_t index;
			uint32_t begin;
			uint32_t length;
		} request;

		struct
		{
			uint32_t index;
			uint32_t begin;
			std::vector<char> block;
		} piece;

		uint16_t port;
		uint16_t messageSize = 0;

		static PeerMessage loadMessage(std::vector<char>& data);
	};
}

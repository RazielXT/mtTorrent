#pragma once
#include <vector>
#include "TcpStream.h"
#include "TorrentFileParser.h"
#include "PeerMessage.h"
#include "Interface.h"

namespace Torrent
{
	struct PeerState
	{
		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;
	};

	struct PiecesBitfield
	{
		std::vector<char> bitfield;
		size_t piecesCount = 0;

		void prepare(size_t bitsize, size_t pieces);
		float getPercentage();
		void addPiece(size_t index);
	};

	class PeerCommunication
	{
	public:

		PeerState state;

		PeerInfo peerInfo;
		TorrentInfo* torrent;
		ClientInfo* client;

		PiecesBitfield pieces;

		void start(TorrentInfo* torrent, ClientInfo* client, PeerInfo peerInfo);

		bool handshake(PeerInfo& peerInfo);

		bool communicate();

		std::vector<char> getHandshakeMessage();

		PeerMessage readNextStreamMessage();
		TcpStream stream;

		void sendInterested();

		void handleMessage(PeerMessage& msg);
	};

}
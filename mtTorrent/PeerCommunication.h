#pragma once
#include <vector>
#include "TcpStream.h"
#include "BencodeParser.h"
#include "PeerMessage.h"
#include "Interface.h"
#include "Extensions.h"

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

	class PeerCommunication
	{
	public:

		PeerExchangeExtension pex;

		PeerState state;

		PeerInfo peerInfo;
		TorrentInfo* torrent;
		ClientInfo* client;

		PiecesProgress pieces;

		void start(TorrentInfo* torrent, ClientInfo* client, PeerInfo peerInfo);

		bool handshake(PeerInfo& peerInfo);

		bool communicate();

		std::vector<char> getHandshakeMessage();

		PeerMessage readNextStreamMessage();
		TcpStream stream;

		void sendInterested();

		void sendBlockRequest(PieceBlockInfo& block);

		void handleMessage(PeerMessage& msg);
	};

}
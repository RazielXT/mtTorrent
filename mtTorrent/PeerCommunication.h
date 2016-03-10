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

	class PeerCommunication
	{
	public:

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

		void handleMessage(PeerMessage& msg);
	};

}
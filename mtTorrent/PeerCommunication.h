#pragma once
#include <vector>
#include "TcpStream.h"
#include "TorrentFileParser.h"
#include "PeerMessage.h"

namespace Torrent
{
	struct PeerInfo
	{
		uint32_t ip;
		std::string ipStr;

		uint16_t port;
		uint16_t index;
	};

	struct PeerState
	{
		uint16_t index;
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
		TorrentFileInfo* torretFile;

		char* peerId;
		PiecesBitfield pieces;

		void start(TorrentFileInfo* torrent, char* peerId, PeerInfo peerInfo);

		bool handshake(PeerInfo& peerInfo);

		bool communicate();

		std::vector<char> getHandshakeMessage();

		PeerMessage readNextStreamMessage();
		TcpStream stream;

		void sendInterested();

		void handleMessage(PeerMessage& msg);
	};

}
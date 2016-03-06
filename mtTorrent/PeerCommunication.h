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
		uint8_t ipAddr[4];
		std::string ipStr;

		uint16_t port;
		uint16_t index;
	};

	class PeerCommunication
	{
	public:

		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;

		PeerInfo peerInfo;
		TorrentFileInfo* torretFile;
		char* peerId;

		void start(TorrentFileInfo* torrent, char* peerId, PeerInfo peerInfo);

		bool handshake(PeerInfo& peerInfo);
		bool communicate();

		std::vector<char> getHandshakeMessage();

		PeerMessage getNextStreamMessage();
		TcpStream stream;

		void setInterested();

		void handleMessage(PeerMessage& msg);
	};

}
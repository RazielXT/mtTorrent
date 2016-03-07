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

	struct PeerState
	{
		uint16_t index;
		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;

		void pushMessage(PeerMessage msg);
		std::vector<PeerMessage> popMessages();

		bool finished = false;

	private:

		std::vector<PeerMessage> messages;
		std::mutex messages_mutex;
	};

	class PeerCommunication
	{
	public:

		PeerState state;

		PeerInfo peerInfo;
		TorrentFileInfo* torretFile;
		char* peerId;

		void start(TorrentFileInfo* torrent, char* peerId, PeerInfo peerInfo);

		bool handshake(PeerInfo& peerInfo);

		bool communicate();
		void startListening();

		std::vector<char> getHandshakeMessage();

		PeerMessage readNextStreamMessage();
		TcpStream stream;

		void sendInterested();

		void handleMessage(PeerMessage& msg);
	};

}
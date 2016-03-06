#pragma once

#include "TorrentFileParser.h"
#include "Network.h"
#include "TcpStream.h"

namespace Torrent
{
	struct AnnounceResponsePeer
	{
		uint32_t ip;
		uint8_t ipAddr[4];
		std::string ipStr;

		uint16_t port;
		uint16_t index;
	};

	struct AnnounceResponse
	{
		uint32_t action;
		uint32_t transaction;
		uint32_t interval;

		uint32_t leechersCount;
		uint32_t seedersCount;

		std::vector<AnnounceResponsePeer> peers;
	};

	struct ConnectMessage
	{
		uint64_t connectionId;
		uint32_t action;
		uint32_t transactionId;
	};

	enum Action
	{
		Connnect = 0,
		Announce,
		Scrape,
		Error
	};

	enum Event
	{
		None = 0,
		Completed,
		Started,
		Stopped
	};

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

	class PeerCommunication
	{
	public:

		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;

		AnnounceResponsePeer peerInfo;
		TorrentFileParser* torretFile;
		char* peerId;

		void start(TorrentFileParser* torrent, char* peerId, AnnounceResponsePeer peerInfo);

		bool handshake(AnnounceResponsePeer& peerInfo);
		bool communicate();

		std::vector<char> getHandshakeMessage();

		PeerMessage getNextStreamMessage();
		TcpStream stream;

		void setInterested();

		void handleMessage(PeerMessage& msg);
	};

	class Communicator
	{
	public:

		void test();

		TorrentFileParser torrent;

		AnnounceResponse announceUdpTracker(std::string host, std::string port);

		std::vector<char> getAnnouncingRequest(ConnectMessage& response);
		std::vector<char> getConnectRequest();

		char peerId[20];
		void generatePeerId();
		
		uint32_t key = 64699946;
		void initIds();

		uint32_t generateTransaction();


		uint32_t listenPort = 80;
		uint32_t maxPeers = 50;
	};
}

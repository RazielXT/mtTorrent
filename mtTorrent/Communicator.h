#pragma once

#include "TorrentFileParser.h"
#include "Network.h"
#include "TcpStream.h"
#include "PeerCommunication.h"

namespace Torrent
{
	struct AnnounceResponse
	{
		uint32_t action;
		uint32_t transaction;
		uint32_t interval;

		uint32_t leechersCount;
		uint32_t seedersCount;

		std::vector<PeerInfo> peers;
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
		uint32_t maxPeers = 40;
	};
}

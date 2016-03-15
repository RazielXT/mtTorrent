#pragma once

#include "Interface.h"
#include "BencodeParser.h"
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

	struct ConnectResponse
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

	struct TrackerCollector
	{
		TrackerCollector(ClientInfo* client, TorrentInfo* tInfo);

		std::vector<PeerInfo> announceAll();

		ClientInfo* client;
		TorrentInfo* torrent;
	};

	class TrackerCommunication
	{
	public:

		TrackerCommunication();
		void setInfo(ClientInfo* client, TorrentInfo* tInfo);

		AnnounceResponse announceUdpTracker(std::string host, std::string port);

		DataBuffer getAnnouncingRequest(ConnectResponse& response);
		DataBuffer getConnectRequest();

		ClientInfo* client;
		TorrentInfo* torrent;
	};
}
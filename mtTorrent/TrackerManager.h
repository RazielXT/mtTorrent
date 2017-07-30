#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"
#include "ServiceThreadpool.h"

namespace mtt
{
	class TrackerListener
	{
	public:

		virtual void onAnnounceResult(AnnounceResponse&, TorrentFileInfo*) = 0;
	};

	struct TrackerManager
	{
	public:

		TrackerManager(boost::asio::io_service& io, TrackerListener& listener);

		void init(TorrentFileInfo* info);
		void addTrackers(std::vector<std::string> trackers);

		void start();
		void stop();

	private:

		std::mutex trackersMutex;
		void onAnnounce(AnnounceResponse&, Tracker*);
		void onTrackerFail(Tracker*);

		TrackerListener& listener;
		TorrentFileInfo* torrent;

		struct TrackerInfo
		{
			std::shared_ptr<Tracker> comm;
			std::string protocol;
			std::string host;
			std::string port;
		};
		std::vector<TrackerInfo> trackers;

		boost::asio::io_service& io;
	};
}
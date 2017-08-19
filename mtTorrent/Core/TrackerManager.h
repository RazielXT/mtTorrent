#pragma once

#include "utils/BencodeParser.h"
#include "utils/ServiceThreadpool.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"
#include "utils/ScheduledTimer.h"

namespace mtt
{
	struct TrackerStateInfo
	{
		std::string host;
		std::string fullAddress;

		Tracker::State state;
		uint32_t updateInterval;
		uint32_t nextUpdate;

		uint32_t peers;
		uint32_t seeds;
		uint32_t leechers;
	};

	class TrackerListener
	{
	public:

		virtual void trackerStateChanged(TrackerStateInfo&, TorrentPtr) = 0;

		virtual void onAnnounceResult(AnnounceResponse&, TorrentPtr) = 0;
	};

	struct TrackerManager
	{
	public:

		TrackerManager(TorrentPtr torrent, std::vector<std::string> trackers, TrackerListener& listener, boost::asio::io_service& io, UdpAsyncComm& udp);

		void start();
		void stop();

	private:

		struct TrackerInfo
		{
			std::shared_ptr<Tracker> comm;
			std::shared_ptr<ScheduledTimer> timer;

			std::string protocol;
			std::string host;
			std::string port;
			bool httpFallback = false;

			bool httpFallbackUsed = false;
			uint32_t retryCount = 0;

			TrackerStateInfo getStateInfo();
		};
		std::vector<TrackerInfo> trackers;
		std::mutex trackersMutex;

		void addTracker(std::string addr);
		void start(TrackerInfo*);
		void startNext();
		void stopAll();
		TrackerInfo* findTrackerInfo(Tracker*);
		TrackerInfo* findTrackerInfo(std::string host);

		void onAnnounce(AnnounceResponse&, Tracker*);
		void onTrackerFail(Tracker*);

		TorrentPtr torrent;
		boost::asio::io_service& io;
		TrackerListener& listener;
		UdpAsyncComm& udp;
	};
}
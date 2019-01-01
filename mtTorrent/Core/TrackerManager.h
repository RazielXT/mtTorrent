#pragma once

#include "utils/ServiceThreadpool.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"
#include "utils/ScheduledTimer.h"

namespace mtt
{
	struct TrackerManager
	{
	public:

		TrackerManager(TorrentPtr t);

		using AnnounceCallback = std::function<void(Status, AnnounceResponse*, Tracker*)>;
		void start(AnnounceCallback announceCallback);
		void stop();

		void addTracker(std::string addr);
		void addTrackers(const std::vector<std::string>& trackers);
		void removeTracker(const std::string& addr);
		void removeTrackers();

		std::shared_ptr<Tracker> getTracker(const std::string& addr);
		std::vector<std::shared_ptr<Tracker>> getTrackers();

		uint32_t getTrackersCount();

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
		};
		std::vector<TrackerInfo> trackers;
		std::mutex trackersMutex;

		void start(TrackerInfo*);
		void startNext();
		void stopAll();
		TrackerInfo* findTrackerInfo(Tracker*);
		TrackerInfo* findTrackerInfo(std::string host);

		void onAnnounce(AnnounceResponse&, Tracker*);
		void onTrackerFail(Tracker*);

		TorrentPtr torrent;
		AnnounceCallback announceCallback;
	};
}
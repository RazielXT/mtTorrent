#pragma once

#include "PeerCommunication.h"
#include "UdpTrackerComm.h"
#include "utils/ScheduledTimer.h"
#include "utils/Uri.h"

namespace mtt
{
	struct TrackerManager
	{
	public:

		TrackerManager(Torrent& t);

		using AnnounceCallback = std::function<void(Status, const AnnounceResponse*, const Tracker&)>;
		void start(AnnounceCallback announceCallback);
		void stop();

		void addTracker(std::string addr);
		void addTrackers(const std::vector<std::string>& trackers);
		void removeTrackers();

		std::shared_ptr<Tracker> getTrackerByAddr(const std::string& addr);
		std::vector<std::pair<std::string,std::shared_ptr<Tracker>>> getTrackers() const;

		uint32_t getTrackersCount();

	private:

		struct TrackerInfo
		{
			std::shared_ptr<Tracker> comm;
			std::shared_ptr<ScheduledTimer> timer;

			Uri uri;
			std::string fullAddress;
			bool httpFallback = false;

			bool httpFallbackUsed = false;
			uint32_t retryCount = 0;
		};
		std::vector<TrackerInfo> trackers;
		mutable std::mutex trackersMutex;

		bool start(TrackerInfo*);
		void startNext();
		void stopAll();
		TrackerInfo* findTrackerInfo(Tracker*);
		TrackerInfo* findTrackerInfo(std::string host);

		void onAnnounce(AnnounceResponse&, Tracker*);
		void onTrackerFail(Tracker*);

		Torrent& torrent;
		AnnounceCallback announceCallback;
	};
}
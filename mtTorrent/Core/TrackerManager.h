#pragma once

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

		virtual void trackerStateChanged(TrackerStateInfo&, CorePtr) = 0;

		virtual void onAnnounceResult(AnnounceResponse&, CorePtr) = 0;
	};

	struct TrackerManager
	{
	public:

		TrackerManager();

		void init(CorePtr core, TrackerListener* listener);
		void start();
		void stop();

		void addTracker(std::string addr);
		void addTrackers(const std::vector<std::string>& trackers);

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

		void start(TrackerInfo*);
		void startNext();
		void stopAll();
		TrackerInfo* findTrackerInfo(Tracker*);
		TrackerInfo* findTrackerInfo(std::string host);

		void onAnnounce(AnnounceResponse&, Tracker*);
		void onTrackerFail(Tracker*);

		CorePtr core;
		TrackerListener* listener;
		UdpCommPtr udp;
	};
}
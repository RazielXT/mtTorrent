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

		virtual void onAnnounceResult(AnnounceResponse&, TorrentPtr) = 0;
	};

	struct TrackerTimer : public std::enable_shared_from_this<TrackerTimer>
	{
		static std::shared_ptr<TrackerTimer> create(boost::asio::io_service& io, std::function<void()> callback);

		TrackerTimer(boost::asio::io_service& io, std::function<void()> callback);

		void schedule(uint32_t secondsOffset);
		void disable();

	private:

		void checkTimer();
		std::function<void()> func;
		boost::asio::deadline_timer timer;
	};

	struct TrackerManager
	{
	public:

		TrackerManager();

		void add(TorrentPtr torrent, std::vector<std::string> trackers, TrackerListener& listener);

		void start(TorrentPtr torrent);
		void stop(TorrentPtr torrent);

	private:

		struct TorrentTrackers
		{
			TorrentTrackers(boost::asio::io_service& io, TrackerListener& l);

			struct TrackerInfo
			{
				std::shared_ptr<Tracker> comm;
				std::shared_ptr<TrackerTimer> timer;

				std::string protocol;
				std::string host;
				std::string port;
				bool httpFallback = false;

				bool httpFallbackUsed = false;
				uint32_t retryCount = 0;
			};
			std::vector<TrackerInfo> trackers;
			std::mutex trackersMutex;

			void addTracker(std::string addr);
			void start(TrackerInfo*);
			void startNext();
			void stopAll();
			TrackerInfo* findTrackerInfo(Tracker*);
			TrackerInfo* findTrackerInfo(std::string host);

			TorrentPtr torrent;
			boost::asio::io_service& io;
			TrackerListener& listener;

			void onAnnounce(AnnounceResponse&, Tracker*);
			void onTrackerFail(Tracker*);			
		};
		std::vector<std::shared_ptr<TorrentTrackers>> torrents;

		ServiceThreadpool service;
	};
}
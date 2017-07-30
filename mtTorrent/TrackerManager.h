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


	struct TrackerTimer : public std::enable_shared_from_this<UdpAsyncClient>
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
			std::shared_ptr<TrackerTimer> timer;
			uint32_t retryCount = 0;

			std::string protocol;
			std::string host;
			std::string port;
		};
		std::vector<TrackerInfo> trackers;

		boost::asio::io_service& io;
	};
}
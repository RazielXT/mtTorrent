#include <vector>

class AppCore;

/*
Schedule Torrent start call for later date, UI only - state is not saved on exit
*/
class TorrentScheduler
{
public:

	TorrentScheduler(AppCore& core);

	void scheduleTorrent(System::DateTime time, const uint8_t* hash);
	bool isScheduled(const uint8_t* hash);
	//seconds until start or 0
	int getSchedule(const uint8_t* hash);

	void queueTorrent(const uint8_t* hash, bool front);
	bool isQueued(const uint8_t* hash);
	int getQueue(const uint8_t* hash);

	void stopAfterFinish(const uint8_t* hash);

	void update();
	void torrentFinished(const uint8_t* hash);
	void stop(const uint8_t* hash);

private:

	struct ScheduledTorrent
	{
		uint8_t hash[20];
		bool start = true;
		int countdown = 0;
	};
	std::vector<ScheduledTorrent> scheduledTorrents;

	struct TorrentInfo
	{
		uint8_t hash[20];
	};
	std::vector<TorrentInfo> queuedTorrents;

	std::vector<TorrentInfo> stopAfterFinishTorrents;

	AppCore& core;
};

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
	void stopSchedule(const uint8_t* hash);
	//seconds until start or 0
	int getSchedule(const uint8_t* hash);

	void update();

private:

	struct ScheduledTorrent
	{
		uint8_t hash[20];
		bool start = true;
		int countdown = 0;
	};
	std::vector<ScheduledTorrent> scheduledTorrents;

	AppCore& core;
};

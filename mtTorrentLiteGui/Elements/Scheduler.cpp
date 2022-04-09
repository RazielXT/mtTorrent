#include "MainForm.h"
#include "../AppCore.h"

extern AppCore core;
static void ScheduleTorrentUpdateFunc(System::Object^ myObject, System::EventArgs^ myEventArgs)
{
	core.scheduler.update();
}

TorrentScheduler::TorrentScheduler(AppCore& c) : core(c)
{
}

void TorrentScheduler::scheduleTorrent(System::DateTime time, const uint8_t* hash)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			scheduledTorrents.erase(it);
			break;
		}
	}

	ScheduledTorrent t;
	t.countdown = max(0, (int)time.Subtract(System::DateTime::Now).TotalSeconds);
	memcpy(t.hash, hash, 20);

	scheduledTorrents.push_back(t);

	//schedule for 1 second updates if not scheduled yet
	if (!GuiLite::MainForm::instance->schedulerTimer)
	{
		GuiLite::MainForm::instance->schedulerTimer = gcnew System::Windows::Forms::Timer();
		GuiLite::MainForm::instance->schedulerTimer->Tick += gcnew System::EventHandler(ScheduleTorrentUpdateFunc);
		GuiLite::MainForm::instance->schedulerTimer->Interval = 1000;
	}

	GuiLite::MainForm::instance->schedulerTimer->Start();
}

bool TorrentScheduler::isScheduled(const uint8_t* hash)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			return true;
		}
	}

	return false;
}

int TorrentScheduler::getSchedule(const uint8_t* hash)
{
	for (auto& t : scheduledTorrents)
	{
		if (memcmp(t.hash, hash, 20) == 0)
			return t.countdown;
	}

	return 0;
}

void TorrentScheduler::queueTorrent(const uint8_t* hash, bool front)
{
	for (auto it = queuedTorrents.begin(); it != queuedTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			queuedTorrents.erase(it);
			break;
		}
	}

	TorrentInfo t;
	memcpy(t.hash, hash, 20);

	if (front)
		queuedTorrents.insert(queuedTorrents.begin(), t);
	else
		queuedTorrents.push_back(t);
}

bool TorrentScheduler::isQueued(const uint8_t* hash)
{
	for (auto it = queuedTorrents.begin(); it != queuedTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			return true;
		}
	}

	return false;
}

int TorrentScheduler::getQueue(const uint8_t* hash)
{
	for (size_t i = 0; i < queuedTorrents.size(); i++)
	{
		if (memcmp(queuedTorrents[i].hash, hash, 20) == 0)
		{
			return (int)i + 1;
		}
	}

	return 0;
}

void TorrentScheduler::stopAfterFinish(const uint8_t* hash)
{
	for (auto it = stopAfterFinishTorrents.begin(); it != stopAfterFinishTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			return;
		}
	}

	TorrentInfo t;
	memcpy(t.hash, hash, 20);
	stopAfterFinishTorrents.push_back(t);
}

void TorrentScheduler::torrentFinished(const uint8_t* hash)
{
	for (auto it = stopAfterFinishTorrents.begin(); it != stopAfterFinishTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			core.IoctlFunc(mtBI::MessageId::Stop, it->hash, nullptr);
			stopAfterFinishTorrents.erase(it);
			break;
		}
	}

	if (!queuedTorrents.empty())
	{
		auto t = queuedTorrents.front();
		queuedTorrents.erase(queuedTorrents.begin());

		core.IoctlFunc(mtBI::MessageId::Start, t.hash, nullptr);
	}
}

void TorrentScheduler::stop(const uint8_t* hash)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			scheduledTorrents.erase(it);
			break;
		}
	}
	for (auto it = queuedTorrents.begin(); it != queuedTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			queuedTorrents.erase(it);
			break;
		}
	}
	for (auto it = stopAfterFinishTorrents.begin(); it != stopAfterFinishTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			stopAfterFinishTorrents.erase(it);
			break;
		}
	}
}

void TorrentScheduler::update()
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end();)
	{
		it->countdown--;

		if (it->countdown <= 0)
		{
			core.IoctlFunc(mtBI::MessageId::Start, it->hash, nullptr);
			it = scheduledTorrents.erase(it);
		}
		else
			it++;
	}

	if(scheduledTorrents.empty())
		GuiLite::MainForm::instance->schedulerTimer->Stop();
}

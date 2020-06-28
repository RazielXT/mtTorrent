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

int TorrentScheduler::getSchedule(const uint8_t* hash)
{
	for (auto& t : scheduledTorrents)
	{
		if (memcmp(t.hash, hash, 20) == 0)
			return t.countdown;
	}

	return 0;
}

void TorrentScheduler::stopSchedule(const uint8_t* hash)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			scheduledTorrents.erase(it);
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

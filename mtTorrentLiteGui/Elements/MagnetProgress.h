#pragma once
#include "../AppCoreCallbacks.h"
#include "../../mtTorrent/Public/Status.h"

class AppCore;

/*
Magnet metadata download window - progress + logs
*/
class MagnetProgress
{
public:

	MagnetProgress(AppCore& core);

	mtt::Status addTorrentFromMetadata(const char* magnetPtr);

	void update();

	void onButtonClick(ButtonId id);

private:

	enum class Sequence { Closed, Input, Added, AddedWithLogs, Finished };
	struct
	{
		Sequence magnetLinkSequence = Sequence::Closed;
		uint32_t lastMagnetLinkLogCount = 0;
		uint8_t hash[20];
	}
	state;

	AppCore& core;
};
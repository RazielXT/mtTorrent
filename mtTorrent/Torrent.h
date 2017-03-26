#pragma once
#include "TorrentDefines.h"
#include "PeerMgr.h"

namespace mtt
{
	enum class TorrentState
	{
		Stopped, Active
	};

	class Torrent
	{
	public:

		Torrent(std::string filepath);

		void start();
		void stop();

		uint32_t id = 0;
		TorrentFileInfo info;
		TorrentState state;
		std::shared_ptr<ProgressScheduler> scheduler;
		PeerMgr* comm = nullptr;
	};
}

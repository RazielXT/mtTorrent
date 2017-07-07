#pragma once
#include "TorrentDefines.h"
#include "PeerMgr.h"

namespace mtt
{
	struct TorrentInfo
	{
		enum { Stopped, Paused, NoInfo, Active } state = Stopped;

		TorrentFileInfo info;
	};

	class Torrent
	{
	public:

		Torrent(const char* id, const char* params);
		Torrent(const char* file);

		void start();
		void pause();
		void stop();
		void remove();

	private:

		TorrentInfo info;
		PeerMgr comm;
	};
}

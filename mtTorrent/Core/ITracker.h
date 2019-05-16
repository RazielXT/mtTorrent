#pragma once
#include <functional>
#include "Interface.h"

namespace mtt
{
	class Tracker
	{
	public:

		virtual void init(std::string host, std::string port, TorrentPtr torrent) = 0;
		virtual void deinit() = 0;

		virtual void announce() = 0;

		TrackerInfo info;

		std::function<void()> onFail;
		std::function<void(AnnounceResponse&)> onAnnounceResult;

		TorrentPtr torrent;
	};
}

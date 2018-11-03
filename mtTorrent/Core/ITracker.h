#pragma once
#include <functional>
#include "Interface.h"

namespace mtt
{
	class Tracker
	{
	public:

		virtual void init(std::string host, std::string port, CorePtr torrent) = 0;

		virtual void announce() = 0;

		struct  
		{
			std::string hostname;

			uint32_t peers;
			uint32_t seeds;
			uint32_t leechers;
			uint32_t announceInterval;
		}
		info;
		
		enum State{ Clear, Initialized, Alive, Connecting, Connected, Announcing, Announced, Reannouncing } state = Clear;

		std::function<void()> onFail;
		std::function<void(AnnounceResponse&)> onAnnounceResult;

		CorePtr core;
	};
}

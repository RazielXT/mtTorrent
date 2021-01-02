#pragma once

#include "Api/Interface.h"

namespace mttApi
{
	class Peers
	{
	public:

		/*
			Get list of trackers and their current state
		*/
		API_EXPORT std::vector<mtt::TrackerInfo> getSourcesInfo();
		/*
			Force announce tracker by its name
		*/
		API_EXPORT void refreshSource(const std::string& name);

		/*
			Get count of currently connected/found peers
		*/
		API_EXPORT uint32_t connectedCount();
		API_EXPORT uint32_t receivedCount();

		/*
			Force connect to peer with specific address (ip:port)
		*/
		API_EXPORT void connect(const char* address);
	};
}

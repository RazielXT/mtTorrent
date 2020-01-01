#pragma once

#include <Api\Interface.h>

namespace mttApi
{
	class Peers
	{
	public:

		API_EXPORT std::vector<mtt::TrackerInfo> getSourcesInfo();
		API_EXPORT void refreshSource(const std::string& name);

		API_EXPORT uint32_t connectedCount();
		API_EXPORT uint32_t receivedCount();

		API_EXPORT void connect(const char* address);
	};
}

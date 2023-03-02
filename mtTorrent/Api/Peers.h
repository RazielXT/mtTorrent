#pragma once

#include "Interface.h"

namespace mttApi
{
	class Peers
	{
	public:

		/*
			Get list of sources/trackers and their current state
		*/
		API_EXPORT std::vector<mtt::TrackerInfo> getSourcesInfo();
		/*
			Force announce tracker/source by its name
		*/
		API_EXPORT void refreshSource(const std::string& name);
		/*
			Add trackers by its full url eg. udp://path.com:port/announce
		*/
		API_EXPORT mtt::Status addTrackers(const std::vector<std::string>& urls);

		/*
			Get count of currently connected/found peers
		*/
		API_EXPORT uint32_t connectedCount() const;
		API_EXPORT uint32_t receivedCount() const;

		/*
			Force connect to peer with specific address (ip:port)
		*/
		API_EXPORT mtt::Status connect(const char* address);

		/*
			Get list of currently connected peers
		*/
		API_EXPORT std::vector<mtt::ConnectedPeerInfo> getConnectedPeersInfo() const;

	protected:

		Peers() = default;
		Peers(const Peers&) = delete;
	};
}

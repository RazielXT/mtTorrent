#pragma once

#include <Api\Interface.h>

namespace mttApi
{
	class MagnetDownload
	{
	public:

		API_EXPORT mtt::MetadataDownloadState getState();
		API_EXPORT size_t getDownloadLog(std::vector<std::string>& logs, size_t logStart = 0);
		API_EXPORT size_t getDownloadLogSize();

		API_EXPORT std::vector<mtt::ActivePeerInfo> getPeersInfo();
	};
}

#pragma once

#pragma once

#include <Api\Interface.h>

namespace mttApi
{
	class FileTransfer
	{
	public:

		API_EXPORT size_t getDownloadSpeed();
		API_EXPORT size_t getUploadSpeed();

		API_EXPORT std::vector<mtt::ActivePeerInfo> getPeersInfo();

		API_EXPORT std::vector<uint32_t> getCurrentRequests();
		API_EXPORT uint32_t getCurrentRequestsCount();
	};
}

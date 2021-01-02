#pragma once

#include "Api/Interface.h"

namespace mttApi
{
	class FileTransfer
	{
	public:

		/*
			Get bytes downloaded/uploaded last second
		*/
		API_EXPORT uint32_t getDownloadSpeed();
		API_EXPORT uint32_t getUploadSpeed();

		/*
			Get list of currently connected peers
		*/
		API_EXPORT std::vector<mtt::ActivePeerInfo> getPeersInfo();

		/*
			Get list of currently requested pieces
		*/
		API_EXPORT std::vector<uint32_t> getCurrentRequests();
	};
}

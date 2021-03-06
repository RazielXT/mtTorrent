#pragma once

#include "Interface.h"

namespace mttApi
{
	class FileTransfer
	{
	public:

		/*
			Get bytes downloaded/uploaded last second
		*/
		API_EXPORT uint32_t getDownloadSpeed() const;
		API_EXPORT uint32_t getUploadSpeed() const;

		/*
			Get list of currently connected peers
		*/
		API_EXPORT std::vector<mtt::ActivePeerInfo> getPeersInfo() const;

		/*
			Get list of currently requested pieces
		*/
		API_EXPORT std::vector<uint32_t> getCurrentRequests() const;
	};
}

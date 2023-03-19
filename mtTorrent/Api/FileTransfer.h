#pragma once

#include "Interface.h"

namespace mttApi
{
	class FileTransfer
	{
	public:

		/*
			Get bytes downloaded/uploaded in last second
		*/
		API_EXPORT uint32_t downloadSpeed() const;
		API_EXPORT uint32_t uploadSpeed() const;

		/*
			Get bytes downloaded/uploaded sum
		*/
		API_EXPORT uint64_t downloaded() const;
		API_EXPORT uint64_t uploaded() const;

		/*
			Get list of currently requested pieces
		*/
		API_EXPORT std::vector<uint32_t> getCurrentRequests() const;

		/*
			Get details about currently requested pieces
		*/
		API_EXPORT std::vector<mtt::PieceDownloadState> getCurrentRequestsInfo() const;

	protected:

		FileTransfer() = default;
		FileTransfer(const FileTransfer&) = delete;
	};
}

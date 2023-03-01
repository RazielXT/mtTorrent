#pragma once

#include "Interface.h"

namespace mttApi
{
	class MetadataDownload
	{
	public:

		/*
			Get info about state of downloading torrent metadata from magnet
		*/
		API_EXPORT mtt::MetadataDownloadState getState() const;

		/*
			Get readable info about metadata download, return all logs or start from specific index
		*/
		API_EXPORT std::size_t getDownloadLog(std::vector<std::string>& logs, std::size_t logStart = 0) const;
		API_EXPORT std::size_t getDownloadLogSize() const;

	protected:

		MetadataDownload() = default;
		MetadataDownload(const MetadataDownload&) = delete;
	};
}

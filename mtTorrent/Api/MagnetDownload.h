#pragma once

#include "Interface.h"

namespace mttApi
{
	class MagnetDownload
	{
	public:

		/*
			Get info about state of downloading torrent metadata from magnet
		*/
		API_EXPORT mtt::MetadataDownloadState getState() const;

		/*
			Get readable info about metadata download, return all logs or start from specific index
		*/
		API_EXPORT size_t getDownloadLog(std::vector<std::string>& logs, size_t logStart = 0) const;
		API_EXPORT size_t getDownloadLogSize() const;
	};
}

#pragma once

namespace mtt
{
	enum class AlertCategory
	{
		Torrent = 0x10000,
		Metadata = 0x20000,
		Config = 0x30000,
	};

	enum class AlertId
	{
		TorrentAdded = (int)AlertCategory::Torrent,
		TorrentFinished,

		MetadataUpdated = (int)AlertCategory::Metadata,
		MetadataFinished,

		ConfigChanged = (int)AlertCategory::Config,
	};
}
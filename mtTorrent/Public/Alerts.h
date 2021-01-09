#pragma once

namespace mtt
{
	enum class AlertCategory
	{
		Torrent = 0x10000,
		Metadata = 0x20000,
		Config = 0x40000,
	};

	enum class AlertId
	{
		TorrentAdded = (int)AlertCategory::Torrent,
		TorrentFinished,

		MetadataFinished = (int)AlertCategory::Metadata,

		ConfigChanged = (int)AlertCategory::Config,
	};
}
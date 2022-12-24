#pragma once

#include "../utils/BitUtils.h"

namespace mtt
{
	namespace Alerts
	{
		enum Category
		{
			Torrent =	0x00FFFF,
			Metadata =	0x0F0000,
			Config =	0xF00000,
		};

		enum Id
		{
			TorrentAdded = utils::firstBit(Category::Torrent),
			TorrentFinished = TorrentAdded << 1,

			MetadataFinished = utils::firstBit(Category::Metadata),

			ConfigChanged = utils::firstBit(Category::Config),
		};
	}
}

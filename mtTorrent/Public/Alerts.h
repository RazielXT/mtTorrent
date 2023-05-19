#pragma once

#include <cstdint>

namespace mtt
{
	constexpr uint64_t firstBit(uint64_t number) { uint64_t b = 1; while (!(number & b)) b <<= 1; return b; }

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
			TorrentFinished = firstBit(Category::Torrent),
			TorrentAdded = TorrentFinished << 1,
			TorrentRemoved = TorrentFinished << 2,
			TorrentCheckStarted = TorrentFinished << 3,
			TorrentCheckFinished = TorrentFinished << 4,
			TorrentInterrupted = TorrentFinished << 5,

			MetadataInitialized = firstBit(Category::Metadata),

			ConfigChanged = firstBit(Category::Config),
		};
	}
}

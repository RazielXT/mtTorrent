#pragma once
#include <vector>

namespace mttLib
{
	enum MessageId
	{
		AddTorrentFile,
		GetTorrents,
		GetTorrentsInfo
	};

	struct AddTorrentFileParams
	{
		std::string path;
	};

	struct Torrent
	{
		uint32_t id;
		std::string name;
		bool active;
	};

	struct GetTorrentsParams
	{
		std::vector<Torrent> torrents;
	};

	struct TorrentInfo
	{
		Torrent basic;
		float progress;
		uint32_t seeds;
		uint32_t seedsConnected;
		uint32_t peers;
		uint32_t peersConnected;
	};

	struct GetTorrentsParams
	{
		std::vector<TorrentInfo> torrents;
	};
}

#ifdef STANDALONE
typedef void(*IoctlMsg)(mttLib::MessageId, void*);
#endif
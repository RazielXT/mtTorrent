#pragma once
#include <vector>

namespace mttLib
{
	enum ResultId
	{
		Ok,
		CommandFailed,
		BadFile
	};

	enum MessageId
	{
		TorrentChangeNotify,

		AddTorrentFile,
		GetTorrents,
		GetTorrentsInfo
	};

	struct AddTorrentFileParams
	{
		std::string path;

		uint32_t id;
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

	struct TorrentChangeNotifyParams
	{
		uint32_t id;

		enum ChangeType
		{
			State,
			Progress
		};
	};
}

#ifndef STANDALONE
typedef mttLib::ResultId(*mtIoctl)(mttLib::MessageId, void*);
#endif
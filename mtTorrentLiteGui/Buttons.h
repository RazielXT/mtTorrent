#pragma once

enum class ButtonId
{
	AddTorrentFile,
	AddTorrentMagnet,
	Remove,
	Start,
	Stop,
	Settings,
	AddPeerMenu,
	AddPeer,
	TorrentGrid,
	SourceRefresh,
	TorrentDoubleClick,
	SelectFiles,
	SelectionOk,
	SelectionCancel,
	SelectionAll,
	SelectionNone,
	MagnetButton,
	OpenLocation,
	Schedule,
	MagnetLogs,
	CheckFiles,
};

extern void onButtonClick(ButtonId id);
extern void onButtonClick(ButtonId id, System::String^ param);

struct TorrentCtxMenuInfo
{
	bool active;
	bool utmLogs;
	bool noInfo;
};
extern TorrentCtxMenuInfo getTorrentContexMenuInfo();

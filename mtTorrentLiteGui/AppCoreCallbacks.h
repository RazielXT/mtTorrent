#pragma once
#include <vector>

//callbacks called from forms 

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

extern void applySettings(System::Windows::Forms::Form^ form);
extern System::String^ getUpnpInfo();
extern void scheduleTorrent(System::DateTime time);
extern void fillFilesSelectionForm();
extern void fileSelectionChanged(int id, bool selected);
extern void filePriorityChanged(const std::vector<int>& ids, System::String^ priority);
extern System::String^ fileSelectionValidatePath(System::String^ path);
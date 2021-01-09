#pragma once
#include <stdint.h>
#include "MttLibrary.h"
#include "Elements/FileSelection.h"
#include "Elements/MagnetProgress.h"
#include "Elements/Settings.h"
#include "Elements/TorrentsView.h"
#include "Elements/Scheduler.h"
#include "../AppCoreCallbacks.h"

/*
core object containing all application logic objects and mtt library comunication
*/
class AppCore : public MttLibrary
{
public:

	AppCore();

	bool init();
	void deinit();

	//process arguments, currently magnet/filepath is supported
	void processProgramArgument(System::String^ arg);
	//bring main window to front
	void showWindow();

	void update();
	void onButtonClick(ButtonId id, System::String^ param);

	void addTorrentFromFile();
	//communicate add result if necessary
	void handleTorrentAddResponse(mtt::Status status, uint8_t* hash);
	void removeTorrent(uint8_t* hash, bool removeFiles);

	bool selected = false;
	bool selectionChanged = false;
	//hash of first selected torrent
	uint8_t firstSelectedHash[20];

	//get state of selected torrent relevant for context menu
	TorrentCtxMenuInfo getTorrentContexMenuInfo();
	System::String^ getUpnpInfo();

	//specific elements logic
	FileSelection fileSelection;
	MagnetProgress magnetProgress;
	TorrentsView torrentsView;
	TorrentScheduler scheduler;
	Settings settings;

private:

	static void stopRunningTorrents();

	System::String^ getTorrentName(uint8_t* hash);

	mtt::Status addTorrentFromFile(const char* filepath);

	void registerAlerts();
	void checkAlerts();

	int RefreshTimeCounter = 10;
	bool forceRefresh = true;
};

#pragma once

#include "../Utils/Utils.h"
#include "FileSelectionForm.h"
#include "../../mtTorrent/Public/BinaryInterface.h"

class AppCore;

/*
Torrent files list with selection/priority/location
*/
class FileSelection
{
public:

	FileSelection(AppCore& core);

	void fileSelectionChanged(int id, bool selected);

	/*
	priority of selected files changed
	*/
	void filePriorityChanged(const std::vector<int>& filesIdx, System::String^ value);

	/*
	load files info for currently selected torrent to files form
	*/
	void fillFilesSelectionForm();

	/*
	open selection window, added=true if torrent was just added to application
	*/
	void showFilesSelectionForm(const uint8_t* hash, bool added);

	void onButtonClick(ButtonId id);

private:

	struct
	{
		mtBI::TorrentInfo info;
		uint8_t hash[20];
		bool added = false;
		bool priorityChanged = false;
	}
	state;

	AppCore& core;

	void updateSelectionFormFooter();
	void setSelectionForAllFiles(bool selected);
};

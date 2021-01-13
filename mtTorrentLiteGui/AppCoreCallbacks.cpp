#include "AppCoreCallbacks.h"
#include "AppCore.h"
#include <fstream>

extern AppCore core;

void onButtonClick(ButtonId id)
{
	onButtonClick(id, nullptr);
}
void onButtonClick(ButtonId id, System::String^ param)
{
	core.onButtonClick(id, param);
}
TorrentCtxMenuInfo getTorrentContexMenuInfo()
{
	return core.getTorrentContexMenuInfo();
}
void applySettings(System::Windows::Forms::Form^ form)
{
	core.settings.applySettings((GuiLite::SettingsForm^)form);
}
System::String^ getUpnpInfo()
{
	return core.getUpnpInfo();
}
void scheduleTorrent(System::DateTime time)
{
	core.scheduler.scheduleTorrent(time, core.firstSelectedHash);
}
void fillFilesSelectionForm()
{
	core.fileSelection.fillFilesSelectionForm();
}
void fileSelectionChanged(int id, bool selected)
{
	core.fileSelection.fileSelectionChanged(id, selected);
}
void filePriorityChanged(const std::vector<int>& ids, System::String^ priority)
{
	core.fileSelection.filePriorityChanged(ids, priority);
}
void fileProgressSelectionChanged(int row)
{
	core.torrentsView.piecesProgress.fileSelectionChanged(row);
}
void forceGuiRefresh()
{
	core.forceRefresh = true;
}
System::String^ fileSelectionValidatePath(System::String^ path)
{
	return core.fileSelection.validatePath(path);
}
void torrentsGridSorted()
{
	core.torrentsView.updateList();
}

SavedWindowState getSavedWindowState()
{
	SavedWindowState state = {};

	std::ifstream file("./window", std::ios_base::binary);
	if (file)
		file.read((char*)&state, sizeof(state));

	return state;
}

void saveWindowState(const SavedWindowState& state)
{
	if (!state.height)
		return;

	std::ofstream file("./window", std::ios_base::binary);
	file.write((const char*)&state, sizeof(state));
}

float bytesToNumber(System::String^ str)
{
	float value = 0;
	int i = str->IndexOf(' ');
	if (i != -1)
		float::TryParse(str->Substring(0, i), value);

	if (str->Contains("GB"))
		value *= 1024 * 1024 * 1024;
	else if (str->Contains("MB"))
		value *= 1024 * 1024;
	else if (str->Contains("KB"))
		value *= 1024;

	return value;
}

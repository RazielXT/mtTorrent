#include "AppCoreCallbacks.h"
#include "AppCore.h"

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
extern void forceGuiRefresh()
{
	core.forceRefresh = true;
}
System::String^ fileSelectionValidatePath(System::String^ path)
{
	return core.fileSelection.validatePath(path);
}
extern void torrentsGridSorted()
{
	core.torrentsView.updateList();
}
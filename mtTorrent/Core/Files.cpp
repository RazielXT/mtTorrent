#include "Files.h"
#include "Configuration.h"

void mtt::Files::init(TorrentInfo& info)
{
	storage.init(info, mtt::config::getExternal().files.defaultDirectory);

	for (auto& f : info.files)
	{
		selection.files.push_back({ true, Priority::Normal, f });
	}

	progress.init(info.pieces.size());
}

void mtt::Files::select(DownloadSelection& s)
{
	selection = s;
	progress.select(selection);
}

bool mtt::Files::select(uint32_t idx, bool selected)
{
	if (selection.files.size() <= idx)
		return false;

	selection.files[idx].selected = selected;
	progress.select(selection);

	return true;
}

mtt::Status mtt::Files::prepareSelection()
{
	return storage.preallocateSelection(selection);
}

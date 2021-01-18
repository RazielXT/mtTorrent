#include "Files.h"
#include "Configuration.h"

mtt::Files::Files(const TorrentInfo& info) : storage(info)
{
}

void mtt::Files::setDefaults(const TorrentInfo& info)
{
	storage.init(mtt::config::getExternal().files.defaultDirectory);

	selection.clear();
	for (auto& f : info.files)
	{
		selection.push_back({ true, Priority::Normal });
	}

	progress.init(info.pieces.size());
}

void mtt::Files::initialize(DownloadSelection s, const std::string& location)
{
	storage.init(location);

	selection = std::move(s);

	progress.calculatePieces();
}

bool mtt::Files::select(const TorrentInfo& info, uint32_t idx, bool selected)
{
	if (selection.size() <= idx)
		return false;

	if (selection[idx].selected != selected)
	{
		selection[idx].selected = selected;
		progress.select(info.files[idx], selected);
	}

	return true;
}

bool mtt::Files::select(const TorrentInfo& info, const std::vector<bool>& s)
{
	if (selection.size() != s.size())
		return false;

	for (size_t i = 0; i < s.size(); i++)
		selection[i].selected = s[i];

	progress.select(info, selection);

	return true;
}

mtt::Status mtt::Files::prepareSelection()
{
	return storage.preallocateSelection(selection);
}

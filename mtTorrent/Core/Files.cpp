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

mtt::Status mtt::Files::addPiece(DownloadedPiece& piece)
{
	auto s = storage.storePiece(piece);

	if (s == Status::Success)
	{
		freshPieces.push_back(piece.index);
		progress.addPiece(piece.index);
	}

	return s;
}

void mtt::Files::select(DownloadSelection& s)
{
	selection = s;
	progress.select(selection);
}

mtt::Status mtt::Files::prepareSelection()
{
	return storage.preallocateSelection(selection);
}

#include "Torrent.h"
#include "Peers.h"
#include "FileTransfer.h"
#include "MetadataDownload.h"

bool mttApi::Torrent::start()
{
	return static_cast<mtt::Torrent*>(this)->start();
}

void mttApi::Torrent::stop()
{
	static_cast<mtt::Torrent*>(this)->stop();
}

mttApi::Torrent::State mttApi::Torrent::getStatus()
{
	return static_cast<mtt::Torrent*>(this)->state;
}

mtt::Status mttApi::Torrent::getLastError()
{
	return static_cast<mtt::Torrent*>(this)->lastError;
}

float mttApi::Torrent::checkingProgress()
{
	return static_cast<mtt::Torrent*>(this)->checkingProgress();
}

bool mttApi::Torrent::selectFiles(std::vector<bool>& s)
{
	return static_cast<mtt::Torrent*>(this)->selectFiles(s);
}

mtt::DownloadSelection mttApi::Torrent::getFilesSelection()
{
	return static_cast<mtt::Torrent*>(this)->files.selection;
}

std::string mttApi::Torrent::getLocationPath()
{
	auto path = static_cast<mtt::Torrent*>(this)->files.storage.getPath();

	auto& info = getFileInfo().info;
	if (info.files.size() > 1)
		path += info.name;

	return path;
}

std::string mttApi::Torrent::name()
{
	return static_cast<mtt::Torrent*>(this)->name();
}

float mttApi::Torrent::currentProgress()
{
	return static_cast<mtt::Torrent*>(this)->currentProgress();
}

float mttApi::Torrent::currentSelectionProgress()
{
	return static_cast<mtt::Torrent*>(this)->currentSelectionProgress();
}

size_t mttApi::Torrent::downloaded()
{
	return static_cast<mtt::Torrent*>(this)->downloaded();
}

size_t mttApi::Torrent::uploaded()
{
	return static_cast<mtt::Torrent*>(this)->uploaded();
}

size_t mttApi::Torrent::dataLeft()
{
	return static_cast<mtt::Torrent*>(this)->dataLeft();
}

bool mttApi::Torrent::finished()
{
	return static_cast<mtt::Torrent*>(this)->finished();
}

bool mttApi::Torrent::selectionFinished()
{
	return static_cast<mtt::Torrent*>(this)->selectionFinished();
}

const mtt::TorrentFileInfo& mttApi::Torrent::getFileInfo()
{
	return static_cast<mtt::Torrent*>(this)->infoFile;
}

std::shared_ptr<mttApi::Peers> mttApi::Torrent::getPeers()
{
	return static_cast<mtt::Torrent*>(this)->peers;
}

std::shared_ptr<mttApi::FileTransfer> mttApi::Torrent::getFileTransfer()
{
	return std::static_pointer_cast<mttApi::FileTransfer>(static_cast<mtt::Torrent*>(this)->fileTransfer);
}

void mttApi::Torrent::getPiecesBitfield(std::vector<uint8_t>& bitfield)
{
	static_cast<mtt::Torrent*>(this)->files.progress.toBitfield(bitfield);
}

void mttApi::Torrent::getPiecesReceivedList(std::vector<uint32_t>& list)
{
	auto& progress = static_cast<mtt::Torrent*>(this)->files.progress;
	list.reserve(progress.getReceivedPiecesCount());

	for (uint32_t i = 0; i < progress.pieces.size(); i++)
		if (progress.hasPiece(i))
			list.push_back(i);
}

bool mttApi::Torrent::getMetadataDownloadState(mtt::MetadataDownloadState& state)
{
	if (auto utm = static_cast<mtt::Torrent*>(this)->utmDl.get())
	{
		state = utm->state;

		return true;
	}

	return false;
}

bool mttApi::Torrent::getMetadataDownloadLog(std::vector<std::string>& logs, size_t logStart)
{
	if (auto utm = static_cast<mtt::Torrent*>(this)->utmDl.get())
	{
		auto& events = utm->getEvents();
		for (size_t i = logStart; i < events.size(); i++)
		{
			logs.push_back(events[i].toString());
		}

		return true;
	}

	return false;
}

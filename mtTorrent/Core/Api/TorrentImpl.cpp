#include "Torrent.h"
#include "Peers.h"
#include "FileTransfer.h"
#include "MetadataDownload.h"

mtt::Status mttApi::Torrent::start()
{
	return static_cast<mtt::Torrent*>(this)->start();
}

void mttApi::Torrent::stop()
{
	static_cast<mtt::Torrent*>(this)->stop();
}

bool mttApi::Torrent::started() const
{
	return static_cast<const mtt::Torrent*>(this)->started;
}

mttApi::Torrent::State mttApi::Torrent::getState() const
{
	return static_cast<const mtt::Torrent*>(this)->getState();
}

mttApi::Torrent::TimePoint mttApi::Torrent::getStateTimestamp() const
{
	return static_cast<const mtt::Torrent*>(this)->getStateTimestamp();
}

mtt::Status mttApi::Torrent::getLastError() const
{
	return static_cast<const mtt::Torrent*>(this)->lastError;
}

const std::vector<mtt::File>& mttApi::Torrent::getFilesInfo()
{
	return getMetadata().info.files;
}

const std::string& mttApi::Torrent::name() const
{
	return static_cast<const mtt::Torrent*>(this)->name();
}

const uint8_t* mttApi::Torrent::hash() const
{
	return static_cast<const mtt::Torrent*>(this)->hash();
}

float mttApi::Torrent::progress() const
{
	return static_cast<const mtt::Torrent*>(this)->progress();
}

float mttApi::Torrent::selectionProgress() const
{
	return static_cast<const mtt::Torrent*>(this)->selectionProgress();
}

uint64_t mttApi::Torrent::finishedBytes() const
{
	return static_cast<const mtt::Torrent*>(this)->finishedBytes();
}

uint64_t mttApi::Torrent::finishedSelectedBytes() const
{
	return static_cast<const mtt::Torrent*>(this)->finishedSelectedBytes();
}

bool mttApi::Torrent::finished() const
{
	return static_cast<const mtt::Torrent*>(this)->finished();
}

bool mttApi::Torrent::selectionFinished() const
{
	return static_cast<const mtt::Torrent*>(this)->selectionFinished();
}

mtt::Timestamp mttApi::Torrent::getTimeAdded() const
{
	return static_cast<const mtt::Torrent*>(this)->getTimeAdded();
}

const mtt::TorrentFileMetadata& mttApi::Torrent::getMetadata()
{
	auto torrent = static_cast<mtt::Torrent*>(this);
	torrent->loadFileInfo();

	return torrent->infoFile;
}

bool mttApi::Torrent::hasMetadata() const
{
	auto torrent = static_cast<const mtt::Torrent*>(this);
	return !torrent->utmDl || torrent->utmDl->state.finished;
}

mttApi::Files& mttApi::Torrent::getFiles() const
{
	return (mttApi::Files&)static_cast<const mtt::Torrent*>(this)->files;
}

mttApi::Peers& mttApi::Torrent::getPeers() const
{
	return *static_cast<const mtt::Torrent*>(this)->peers;
}

mttApi::FileTransfer& mttApi::Torrent::getFileTransfer() const
{
	return *static_cast<const mtt::Torrent*>(this)->fileTransfer;
}

const mttApi::MetadataDownload* mttApi::Torrent::getMetadataDownload() const
{
	return static_cast<const mtt::Torrent*>(this)->utmDl.get();
}

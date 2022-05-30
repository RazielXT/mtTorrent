#pragma once

#include "../Public/Status.h"
#include "Interface.h"
#include "FileTransfer.h"
#include "Peers.h"
#include "MagnetDownload.h"
#include <memory>
#include <chrono>

namespace mttApi
{
	class Torrent
	{
	public:

		/*
			Start/stop state of torrent, including downloading/uploading/metadata download
			Torrent state is saved automatically
		*/
		API_EXPORT mtt::Status start();
		API_EXPORT void stop();

		enum class ActiveState
		{
			Stopped,
			Started
		};

		/*
			Get active state, that is if torrent tries to connect to other peers
		*/
		API_EXPORT ActiveState getActiveState() const;

		enum class State
		{
			Inactive,				//correctly stopped and not making any work
			Interrupted,			//stopped due to error, see getLastError
			CheckingFiles,			//checking integrity of existing files, see checkingProgress
			DownloadingMetadata,	//downloading metadata from magnet link, see getMagnetDownload
			Downloading,			//active and connecting to peers to download selected files
			Seeding,				//active and accepting incoming connection requests
			Stopping,				//active and in process of stopping
		};

		API_EXPORT State getState() const;

		using TimeClock = std::chrono::system_clock;
		using TimePoint = TimeClock::time_point;

		/*
			get last time when torrent entered active state (not Inactive/Interrupted)
		*/
		API_EXPORT TimePoint getActiveTimestamp() const;
		/*
			get specific error which happened in Interrupted state
		*/
		API_EXPORT mtt::Status getLastError() const;

		/*
			progress of last files checking task (0-1)
		*/
		API_EXPORT float checkingProgress() const;
		/*
			start check of existing files, if not already checking
		*/
		API_EXPORT void checkFiles();

		/*
			get files with current selection info
		*/
		API_EXPORT std::vector<mtt::FileSelection> getFilesSelection() const;
		/*
			get download progress of files, sorted by order in torrent file, % (including unfinished pieces) and finished pieces
		*/
		API_EXPORT std::vector<std::pair<float, uint32_t>> getFilesProgress();
		/*
			get current allocated sizes on disk in bytes, sorted by order in torrent file
		*/
		API_EXPORT std::vector<uint64_t> getFilesAllocatedSize();
		/*
			select/deselect files to download, sorted by order in torrent file
		*/
		API_EXPORT bool selectFiles(const std::vector<bool>&);
		/*
			select/deselect file to download, index sorted by order in torrent file
		*/
		API_EXPORT bool selectFile(uint32_t index, bool selected);
		/*
			set priority of files download, sorted by order in torrent file
		*/
		API_EXPORT void setFilesPriority(const std::vector<mtt::Priority>&);
		/*
			get current download location path
		*/
		API_EXPORT std::string getLocationPath() const;
		/*
			change download location path, moving existing files if wanted
		*/
		API_EXPORT mtt::Status setLocationPath(const std::string& path, bool moveFiles);

		/*
			get name of file from torrent file
		*/
		API_EXPORT const std::string& name() const;
		/*
			get 20 byte torrent hash id
		*/
		API_EXPORT const uint8_t* hash() const;
		/*
			progress of download of all torrent files
		*/
		API_EXPORT float progress() const;
		/*
			progress of download of selected torrent files
		*/
		API_EXPORT float selectionProgress() const;
		/*
			downloaded size in bytes
		*/
		API_EXPORT uint64_t downloaded() const;
		/*
			uploaded size in bytes
		*/
		API_EXPORT uint64_t uploaded() const;
		/*
			downloaded size in bytes including duplicate/invalid data
		*/
		API_EXPORT uint64_t receivedBytes() const;
		/*
			finished state of whole torrent
		*/
		API_EXPORT bool finished() const;
		/*
			finished state of selected torrent files
		*/
		API_EXPORT bool selectionFinished() const;
		/*
			get unit timestamp of date when torrent was added
		*/
		API_EXPORT uint64_t getTimeAdded() const;

		/*
			get loaded torrent file
		*/
		API_EXPORT const mtt::TorrentFileInfo& getFileInfo();

		/*
			see Api\Peers.h
		*/
		API_EXPORT Peers& getPeers();
		/*
			see Api\FileTransfer.h
		*/
		API_EXPORT FileTransfer& getFileTransfer();
		/*
			see Api\MagnetDownload.h
			optional
		*/
		API_EXPORT const MagnetDownload* getMagnetDownload();

		/*
			get count of all pieces
		*/
		API_EXPORT size_t getPiecesCount() const;
		/*
			get pieces progress as bitfield
			in/out dataSize is returned as current bitfield size
		*/
		API_EXPORT bool getPiecesBitfield(uint8_t* dataBitfield, size_t& dataSize) const;
		/*
			get indices of all received pieces
			in/out dataSize is returned as count of received pieces
		*/
		API_EXPORT bool getReceivedPieces(uint32_t* dataPieces, size_t& dataSize) const;
	};
}

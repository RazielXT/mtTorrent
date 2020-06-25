#pragma once

#include "Public/Status.h"
#include "Api/Interface.h"
#include "Api/FileTransfer.h"
#include "Api/Peers.h"
#include "Api/MagnetDownload.h"
#include <memory>


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
			Get active state, that is if torrent tries to achieve connection to other peers
		*/
		API_EXPORT ActiveState getActiveState();

		enum class State
		{
			Inactive,				//correctly stopped and not making any work
			Interrupted,			//stopped due to error, see getLastError
			CheckingFiles,			//active and checking integrity of existing files
			DownloadingMetadata,	//active and downloading metadata from magnet link
			Downloading,			//active and connecting to peers to download selected files
			Seeding,				//active and accepting incoming connection requests
		};

		API_EXPORT State getState();
		/*
			Get specific error which happened in caused Interrupted state
		*/
		API_EXPORT mtt::Status getLastError();

		/*
			progress of last files checking task (0-1)
		*/
		API_EXPORT float checkingProgress();
		/*
			start check of existing files, if not already checking
		*/
		API_EXPORT void checkFiles();

		/*
			get files with current selection info
		*/
		API_EXPORT mtt::DownloadSelection getFilesSelection();
		/*
			get current progress of files, sorted by order in torrent file
		*/
		API_EXPORT std::vector<float> getFilesProgress();
		/*
			select/deselect files to download, sorted by order in torrent file
		*/
		API_EXPORT bool selectFiles(const std::vector<bool>&);
		/*
			set priority of files download, sorted by order in torrent file
		*/
		API_EXPORT void setFilesPriority(const std::vector<mtt::Priority>&);
		/*
			get current download location path
		*/
		API_EXPORT std::string getLocationPath();
		/*
			change download location path, moving existing files
		*/
		API_EXPORT mtt::Status setLocationPath(const std::string& path);

		/*
			get name of file from torrent file
		*/
		API_EXPORT const std::string& name();
		/*
			progress of download of all torrent files
		*/
		API_EXPORT float currentProgress();
		/*
			progress of download of selected torrent files
		*/
		API_EXPORT float currentSelectionProgress();
		/*
			downloaded size in bytes
		*/
		API_EXPORT uint64_t downloaded();
		/*
			uploaded size in bytes
		*/
		API_EXPORT uint64_t uploaded();
		/*
			finished state of whole torrent
		*/
		API_EXPORT bool finished();
		/*
			finished state of selected torrent files
		*/
		API_EXPORT bool selectionFinished();

		/*
			get loaded torrent file
		*/
		API_EXPORT const mtt::TorrentFileInfo& getFileInfo();

		/*
			see Api\Peers.h
		*/
		API_EXPORT std::shared_ptr<Peers> getPeers();
		/*
			see Api\FileTransfer.h
		*/
		API_EXPORT std::shared_ptr<mttApi::FileTransfer> getFileTransfer();
		/*
			see Api\MagnetDownload.h
		*/
		API_EXPORT std::shared_ptr<mttApi::MagnetDownload> getMagnetDownload();

		/*
			get pieces progress as bitfield
			see torrent file info.expectedBitfieldSize
		*/
		API_EXPORT bool getPiecesBitfield(uint8_t* dataBitfield, size_t dataSize);
		/*
			get indices of all received pieces
			dataSize is returned as count of received pieces
		*/
		API_EXPORT bool getReceivedPieces(uint32_t* dataPieces, size_t& dataSize);
	};
}

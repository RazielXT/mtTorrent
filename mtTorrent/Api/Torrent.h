#pragma once

#include "../Public/Status.h"
#include "Interface.h"
#include "FileTransfer.h"
#include "Peers.h"
#include "Files.h"
#include "MetadataDownload.h"
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

		/*
			torrent is started and any stop action havent finished
		*/
		API_EXPORT bool isStarted() const;

		enum class State
		{
			Active,					//connecting to peers to download/upload files
			Stopping,				//in process of stopping
			Stopped,				//correctly stopped and not making any work
			Interrupted,			//stopped due to error, see getLastError
			CheckingFiles,			//checking integrity of existing files, see checkingProgress
			DownloadingMetadata,	//downloading metadata from magnet link, see getMagnetDownload
		};

		API_EXPORT State getState() const;

		using TimeClock = std::chrono::system_clock;
		using TimePoint = TimeClock::time_point;

		/*
			get last time when torrent changed state
		*/
		API_EXPORT TimePoint getStateTimestamp() const;
		/*
			get specific error which happened in Interrupted state
		*/
		API_EXPORT mtt::Status getLastError() const;

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
			get unix timestamp when torrent was added
		*/
		API_EXPORT mtt::Timestamp getTimeAdded() const;

		/*
			check missing info in case of unfinished magnet link
		*/
		API_EXPORT bool hasMetadata() const;
		/*
			get loaded torrent file metadata
		*/
		API_EXPORT const mtt::TorrentFileMetadata& getMetadata();
		/*
			get list of files, from metadata
		*/
		API_EXPORT const std::vector<mtt::File>& getFilesInfo();

		/*
			see Api\Files.h
		*/
		API_EXPORT Files& getFiles() const;
		/*
			see Api\Peers.h
		*/
		API_EXPORT Peers& getPeers() const;
		/*
			see Api\FileTransfer.h
		*/
		API_EXPORT FileTransfer& getFileTransfer() const;
		/*
			see Api\MetadataDownload.h
			optional in case of magnet link
		*/
		API_EXPORT const MetadataDownload* getMetadataDownload() const;
	};
}

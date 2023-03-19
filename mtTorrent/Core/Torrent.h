#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Files.h"
#include "Api/Torrent.h"
#include <functional>

namespace mtt
{
	class MetadataDownload;
	class FileTransfer;
	class Peers;

	class Torrent : public mttApi::Torrent, public std::enable_shared_from_this<Torrent>
	{
	public:

		Torrent();
		~Torrent();

		bool started = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(mtt::TorrentFileMetadata fileInfo);
		static TorrentPtr fromMagnetLink(std::string link);
		static TorrentPtr fromSavedState(std::string name);
		void downloadMetadata();

		bool isActive() const;
		State getState() const;
		TimePoint getStateTimestamp() const;

		mtt::Status start();
		enum class StopReason { Deinit, Manual, Internal };
		void stop(StopReason reason = StopReason::Manual);

		const std::string& name() const;
		float progress() const;
		float selectionProgress() const;
		uint64_t finishedBytes() const;
		uint64_t finishedSelectedBytes() const;
		uint64_t dataLeft() const;
		bool finished() const;
		bool selectionFinished() const;

		const uint8_t* hash() const;
		std::string hashString() const;
		Timestamp getTimeAdded() const;

		Files files;
		TorrentFileMetadata infoFile;
		ServiceThreadpool service;

		std::unique_ptr<Peers> peers;
		std::unique_ptr<FileTransfer> fileTransfer;
		std::unique_ptr<MetadataDownload> utmDl;

		void save();
		void saveTorrentFile(const uint8_t* data, std::size_t size);
		void removeMetaFiles();
		bool loadFileInfo();

		void prepareForChecking();
		void refreshAfterChecking(const PiecesCheck&);

		void refreshSelection();

	protected:

		static bool loadSavedTorrentFile(const std::string& hash, DataBuffer& out);

		Timestamp addedTime = 0;

		bool stateChanged = false;
		TimePoint activityTime;
		void initialize();

		std::mutex stateMutex;
		bool stopping = false;

		enum class LoadedState { Minimal, Full } loadedState = LoadedState::Minimal;
	};
}

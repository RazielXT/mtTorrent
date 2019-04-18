#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Files.h"
#include <functional>

namespace mtt
{
	class MetadataDownload;
	class FileTransfer;
	class Peers;

	class Torrent
	{
	public:

		enum class State
		{
			Stopped,
			Started,
			DownloadUtm,
		}
		state = State::Stopped;
		bool checking = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(std::string filepath);
		static TorrentPtr fromMagnetLink(std::string link);
		void downloadMetadata(std::function<void(Status, MetadataDownloadState&)> callback);

		bool start();
		void pause();
		void stop();

		std::shared_ptr<PiecesCheck> checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish);
		float checkingProgress();
		std::shared_ptr<mtt::PiecesCheck> getCheckState();

		std::string name();
		float currentProgress();
		size_t downloaded();
		size_t downloadSpeed();
		size_t uploaded();
		size_t uploadSpeed();
		size_t dataLeft();
		bool finished();

		uint8_t* hash();

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::unique_ptr<Peers> peers;
		std::unique_ptr<FileTransfer> fileTransfer;
		std::unique_ptr<MetadataDownload> utmDl;

	protected:

		std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;
		void init();
	};
}

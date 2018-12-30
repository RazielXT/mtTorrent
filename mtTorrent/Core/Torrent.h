#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Files.h"
#include <functional>

namespace mtt
{
	class MetadataDownload;
	class Downloader;
	class Peers;

	class Torrent
	{
	public:

		static TorrentPtr fromFile(std::string filepath);
		static TorrentPtr fromMagnetLink(std::string link, std::function<void(Status, MetadataDownloadState&)> callback);

		bool start();
		void pause();
		void stop();

		std::shared_ptr<PiecesCheck> checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish);

		std::string name();
		float currentProgress();
		size_t downloaded();
		size_t downloadSpeed();
		bool finished();

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::unique_ptr<Peers> peers;
		std::unique_ptr<Downloader> downloader;
		std::unique_ptr<MetadataDownload> utmDl;

	protected:

		void init();
	};
}

#pragma once

#include "Torrent.h"
#include "Api/Core.h"
#include "AlertsManager.h"
#include "utils/ScheduledTimer.h"

class TcpAsyncServer;
class ServiceThreadpool;

namespace mtt
{
	namespace dht
	{
		class Communication;
	}

	class IncomingPeersListener;

	class GlobalBandwidth
	{
	public:
		GlobalBandwidth();
		~GlobalBandwidth();
		ServiceThreadpool bwPool;
		std::shared_ptr<ScheduledTimer> bwTimer;
	};

	class Core : public mttApi::Core
	{
	public:

		std::shared_ptr<IncomingPeersListener> listener;
		std::shared_ptr<dht::Communication> dht;

		std::vector<TorrentPtr> torrents;

		void init();
		void deinit();

		std::pair<mtt::Status,TorrentPtr> addFile(const char* filename);
		std::pair<mtt::Status, TorrentPtr> addFile(const uint8_t* data, size_t size);
		std::pair<mtt::Status, TorrentPtr> addMagnet(const char* magnet);

		TorrentPtr getTorrent(const uint8_t* hash);
		TorrentPtr getTorrent(const char* hash);

		Status removeTorrent(const uint8_t* hash, bool deleteFiles);
		Status removeTorrent(const char* hash, bool deleteFiles);

		AlertsManager alerts;
		std::unique_ptr<GlobalBandwidth> bandwidth;
	};
}

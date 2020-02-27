#pragma once

#include "Storage.h"
#include "IPeerListener.h"
#include "Downloader.h"
#include "Uploader.h"
#include "utils/ScheduledTimer.h"
#include "LogFile.h"
#include "Api/FileTransfer.h"

namespace mtt
{
	class FileTransfer : public mttApi::FileTransfer, public IPeerListener
	{
	public:

		FileTransfer(TorrentPtr);

		void start();
		void stop();

		void reevaluate();

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int code) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*, uint32_t) override;

		size_t getDownloadSpeed();
		size_t getUploadSum();
		size_t getUploadSpeed();

		size_t getUnfinishedPiecesDownloadSize();

		std::vector<ActivePeerInfo> getPeersInfo();

		std::vector<uint32_t> getCurrentRequests();
		uint32_t getCurrentRequestsCount();

	private:

#ifdef PEER_DIAGNOSTICS
		struct LogEval
		{
			long time;
			uint32_t count;
		};
		std::vector<LogEval> logEvals;

		struct LogEvalPeer
		{
			uint32_t dl = 0;
			uint32_t up = 0;
			uint32_t activityTime = 0;
			Addr addr;
			enum Action : uint8_t { Keep, TooSoon, NotResponding, TooSlow, Upload } action = Keep;
			char info = 0;
		};
		std::vector<LogEvalPeer> logEvalPeers;

		std::mutex logmtx;
		void saveLogEvents();
#else
		void saveLogEvents() {}
#endif

		std::vector<uint32_t> piecesAvailability;

		std::vector<ActivePeer> activePeers;
		std::mutex peersMutex;

		mtt::ActivePeer* getActivePeer(PeerCommunication* p);
		void addPeer(PeerCommunication*);
		void removePeer(PeerCommunication*);
		void evaluateCurrentPeers();
		void evaluateNextRequests(PeerCommunication*);

		std::shared_ptr<ScheduledTimer> refreshTimer;

		void updateMeasures();
		std::vector<std::pair<PeerCommunication*, std::pair<size_t, size_t>>> lastSpeedMeasure;

		void evalCurrentPeers();
		void removePeers(std::vector<uint32_t> sortedIdx);
		uint32_t peersEvalCounter = 0;

		Downloader downloader;
		Uploader uploader;

		TorrentPtr torrent;

		LogFile log;
	};
}

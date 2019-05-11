#pragma once

#include "Storage.h"
#include "IPeerListener.h"
#include "Downloader.h"
#include "Uploader.h"
#include "utils/ScheduledTimer.h"
#include "LogFile.h"

namespace mtt
{
	class FileTransfer : public IPeerListener
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

		struct ActivePeerInfo
		{
			Addr address;
			std::string client;
			uint32_t uploadSpeed;
			uint32_t downloadSpeed;
			float percentage;
			std::string country;
		};
		std::vector<ActivePeerInfo> getPeersInfo();

	private:

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
		uint32_t peersEvalCounter = 0;

		Downloader downloader;
		Uploader uploader;

		TorrentPtr torrent;

		LogFile log;
	};
}

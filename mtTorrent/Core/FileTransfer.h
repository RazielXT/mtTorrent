#pragma once

#include "Storage.h"
#include "IPeerListener.h"
#include "Downloader.h"
#include "Uploader.h"
#include "utils/ScheduledTimer.h"
#include "Api/FileTransfer.h"

namespace mtt
{
	class FileTransfer : public mttApi::FileTransfer, public IPeerListener
	{
	public:

		FileTransfer(Torrent&);

		void start();
		void stop();

		void refreshSelection();

		std::vector<ActivePeerInfo> getPeersInfo() const;

		Downloader downloader;
		std::shared_ptr<Uploader> uploader;

		uint32_t getDownloadSpeed() const;
		uint32_t getUploadSpeed() const;

	private:

		FileLog log;

		std::vector<uint32_t> piecesAvailability;

		struct ActivePeer
		{
			//todo to stream
			Timestamp connectionTime = 0;
			Timestamp lastActivityTime = 0;

			//todo to stream bandwidth
			uint32_t downloadSpeed = 0;
			uint64_t downloaded = 0;

			uint32_t uploadSpeed = 0;
			uint64_t uploaded = 0;
		};
		std::map<PeerCommunication*, ActivePeer> activePeers;
		mutable std::mutex peersMutex;

		void evaluateMorePeers();
		void addPeer(PeerCommunication* p);
		void removePeer(PeerCommunication* p);

		std::shared_ptr<ScheduledTimer> refreshTimer;

		void updateMeasures();
		std::map<PeerCommunication*, std::pair<uint64_t, uint64_t>> lastSpeedMeasure;
		int32_t updateMeasuresCounter = 0;

		void evalCurrentPeers();
		uint32_t peersEvalCounter = 0;

		Torrent& torrent;

		void handshakeFinished(PeerCommunication*) override;
		void connectionClosed(PeerCommunication*, int code) override;
		void messageReceived(PeerCommunication*, PeerMessage&) override;
		void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) override;
		void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView& data) override;
	};
}

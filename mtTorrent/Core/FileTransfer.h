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

		Downloader downloader;
		std::shared_ptr<Uploader> uploader;

	private:

		FileLog log;

		std::vector<uint32_t> piecesAvailability;

		void evaluateMorePeers();

		std::shared_ptr<ScheduledTimer> refreshTimer;

		void updateMeasures();
		std::map<PeerCommunication*, std::pair<uint64_t, uint64_t>> lastSpeedMeasure;
		std::pair<uint64_t, uint64_t> lastSumMeasure;
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

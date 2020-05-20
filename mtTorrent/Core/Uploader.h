#pragma once
#include "Interface.h"
#include "utils/Bandwidth.h"

namespace mtt
{
	class PeerCommunication;

	class Uploader : public BandwidthUser, public std::enable_shared_from_this<Uploader>
	{
	public:

		Uploader(TorrentPtr);

		void isInterested(PeerCommunication* p);
		void pieceRequest(PeerCommunication* p, PieceBlockInfo& info);
		void refreshRequest();

		uint64_t uploaded = 0;

		std::vector<std::pair<PeerCommunication*, uint32_t>> popHandledRequests();

	private:

		void assignBandwidth(int amount) override;
		void requestBytes(uint32_t amount);
		bool isActive() override;
		void sendRequests();

		std::mutex requestsMutex;

		struct UploadRequest
		{
			std::shared_ptr<PeerCommunication> peer;
			PieceBlockInfo block;
		};
		std::vector<UploadRequest> pendingRequests;

		bool requestingBytes = false;
		uint32_t availableBytes = 0;
		BandwidthChannel* globalBw;

		std::vector<std::pair<PeerCommunication*, uint32_t>> handledRequests;

		TorrentPtr torrent;
	};
}
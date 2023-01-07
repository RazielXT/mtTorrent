#pragma once
#include "Interface.h"
#include "utils/Bandwidth.h"

namespace mtt
{
	class PeerCommunication;

	class Uploader : public BandwidthUser, public std::enable_shared_from_this<Uploader>
	{
	public:

		Uploader(Torrent&);

		void stop();

		void isInterested(PeerCommunication* p);
		void cancelRequests(PeerCommunication* p);
		void pieceRequest(PeerCommunication* p, const PieceBlockInfo& info);
		void refreshRequest();

		uint64_t& getUploadSum();
		uint64_t uploaded = 0;

		std::map<PeerCommunication*, uint32_t> popHandledRequests();

	private:

		std::string name() override;
		void assignBandwidth(int amount) override;
		void requestBytes(uint32_t amount);
		bool isActive() override;
		void sendRequests();

		std::mutex requestsMutex;

		struct UploadRequest
		{
			PeerCommunication* peer;
			PieceBlockInfo block;
		};
		std::vector<UploadRequest> pendingRequests;
		DataBuffer buffer;

		bool requestingBytes = false;
		uint32_t availableBytes = 0;
		BandwidthChannel* globalBw;

		std::map<PeerCommunication*, uint32_t> handledRequests;

		Torrent& torrent;
	};
}
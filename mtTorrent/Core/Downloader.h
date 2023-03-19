#pragma once

#include "DownloadStorage.h"

namespace mtt
{
	class Downloader
	{
	public:

		Downloader(Torrent&);

		void start();
		void stop();

		uint64_t getUnfinishedPiecesDownloadSize();
		uint64_t getUnfinishedSelectedPiecesDownloadSize();
		std::map<uint32_t, uint32_t> getUnfinishedPiecesDownloadSizeMap();

		void handshakeFinished(PeerCommunication*);
		void messageReceived(PeerCommunication*, PeerMessage&);
		void connectionClosed(PeerCommunication*);

		void sortPieces(const std::vector<uint32_t>& availability);

		void refreshSelection(const DownloadSelection& selectedPieces, const std::vector<uint32_t>& availability);

		uint64_t downloaded = 0;
		uint32_t downloadSpeed = 0;

		std::vector<uint32_t> getCurrentRequests() const;
		std::vector<uint32_t> popFreshPieces();

		DownloadStorage storage;
		UnfinishedPiecesState unfinishedPieces;

	private:

		struct PeerRequestsInfo
		{
			struct RequestedPiece
			{
				uint32_t idx;
				uint32_t activeBlocksCount = 0;
				std::vector<bool> blocks;
			};
			std::vector<RequestedPiece> currentRequests;
			std::vector<uint32_t> requestedPieces;

			void received();
			float deltaReceive = 0;
			Torrent::TimePoint lastReceive;

			uint32_t suspiciousGroup = 0;
			uint32_t suspicion = 0;
		};
		std::map<PeerCommunication*, PeerRequestsInfo> peersRequestsInfo;

		void unchokePeer(PeerCommunication*);
		void evaluateNextRequests(PeerRequestsInfo&, PeerCommunication*);

		PeerRequestsInfo* addPeerBlockResponse(PieceBlock& block, PeerCommunication* source);

		struct PieceInfo
		{
			uint32_t idx;
			Priority priority;
		};
		std::vector<PieceInfo> sortedSelectedPieces;

		struct RequestInfo
		{
			PieceState piece;
			uint16_t requestsCounter = 0;
			std::set<PeerCommunication*> activePeers;
			std::set<PeerCommunication*> sources;
			std::set<uint32_t> sourceGroups;
		};
		std::map<uint32_t, std::shared_ptr<RequestInfo>> requests;
		uint32_t GroupIdx = 0;

		void pieceBlockReceived(PieceBlock& block, PeerCommunication* source);
		void pieceChecked(uint32_t, Status, const RequestInfo& info);
		void refreshPieceSources(Status, const RequestInfo& info);

		RequestInfo& getRequest(uint32_t i);
		mutable std::mutex requestsMutex;

		RequestInfo* getBestNextRequest(PeerRequestsInfo&, PeerCommunication*);
		bool fastCheck = true;

		uint32_t checkingPieces = 0;
		std::vector<uint32_t> freshPieces;

		uint32_t sendPieceRequests(PeerCommunication*, PeerRequestsInfo::RequestedPiece&, RequestInfo&, uint32_t max);
		bool hasWantedPieces(PeerCommunication*);
		bool isMissingPiece(uint32_t idx) const;
		void finish();

		uint64_t duplicatedDataSum = 0;

		Torrent& torrent;

		FileLog log;
	};
}

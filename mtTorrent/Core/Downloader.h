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

		size_t getUnfinishedPiecesDownloadSize();
		std::map<uint32_t, uint32_t> getUnfinishedPiecesDownloadSizeMap();

		void handshakeFinished(PeerCommunication*);
		void messageReceived(PeerCommunication*, PeerMessage&);
		void connectionClosed(PeerCommunication*);

		void sortPieces(const std::vector<uint32_t>& availability);

		void refreshSelection(const DownloadSelection& selectedPieces, const std::vector<uint32_t>& availability);

		uint64_t downloaded = 0;

		std::vector<uint32_t> getCurrentRequests() const;
		std::vector<uint32_t> popFreshPieces();

		DownloadStorage storage;
		UnfinishedPiecesState unfinishedPieces;

	private:

		struct PeerRequestsInfo
		{
			PeerCommunication* comm;

			struct RequestedPiece
			{
				uint32_t idx;
				uint32_t activeBlocksCount = 0;
				std::vector<bool> blocks;
			};
			std::vector<RequestedPiece> requestedPieces;

			bool operator==(const mtt::PeerCommunication* p) { return comm == p; }

			void received();
			float deltaReceive = 0;
			Torrent::TimePoint lastReceive;

			std::set<PeerCommunication*> sharedSuspicion;
			bool suspicious() const;
		};
		std::vector<PeerRequestsInfo> peersRequestsInfo;

		void unchokePeer(PeerRequestsInfo*);
		void evaluateNextRequests(PeerRequestsInfo*);

		PeerRequestsInfo* addPeerBlockResponse(PieceBlock& block, PeerCommunication* source);

		std::vector<uint32_t> sortedSelectedPieces;
		std::mutex sortedSelectedPiecesMutex;

		struct RequestInfo
		{
			PieceState piece;
			uint16_t activeRequestsCount = 0;
			bool dontShare = false;
			std::set<PeerCommunication*> sources;
		};
		std::vector<std::shared_ptr<RequestInfo>> requests;

		void pieceBlockReceived(PieceBlock& block, PeerCommunication* source);
		void pieceChecked(uint32_t, Status, const RequestInfo& info);
		void refreshPieceSources(uint32_t, Status, const std::set<PeerCommunication*>& sources);

		RequestInfo* getRequest(uint32_t i);
		RequestInfo* addRequest(uint32_t i);
		mutable std::mutex requestsMutex;

		struct PieceDownloadState
		{
			Priority priority = Priority(0);
			bool missing = false;
			bool request = false;
		};
		std::vector<PieceDownloadState> piecesDlState;

		RequestInfo* getBestNextRequest(PeerRequestsInfo*);
		bool fastCheck = true;

		std::vector<uint32_t> freshPieces;

		uint32_t sendPieceRequests(PeerCommunication*, PeerRequestsInfo::RequestedPiece*, RequestInfo*, uint32_t max);
		bool hasWantedPieces(PeerCommunication*);
		bool isMissingPiece(uint32_t idx);
		void finish();

		uint64_t duplicatedDataSum = 0;

		Torrent& torrent;

		FileLog log;
	};
}

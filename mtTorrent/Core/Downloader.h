#pragma once

#include "PeerCommunication.h"

namespace mtt
{
	struct ActivePeer
	{
		PeerCommunication* comm;

		Timestamp connectionTime = 0;
		Timestamp lastActivityTime = 0;

		uint32_t downloadSpeed = 0;
		uint32_t uploadSpeed = 0;

		uint64_t downloaded = 0;
		uint64_t uploaded = 0;

		struct RequestedPiece
		{
			uint32_t idx;
			std::vector<bool> blocks;
		};
		std::vector<RequestedPiece> requestedPieces;

		uint32_t receivedBlocks = 0;
		uint32_t invalidPieces = 0;
	};

	struct LockedPeers
	{
		LockedPeers(std::vector<ActivePeer>& dt, std::mutex& mtx) : data(dt), mutex(mtx) { mutex.lock(); }
		~LockedPeers() { mutex.unlock(); }

		std::vector<ActivePeer>& get() { return data; }

		ActivePeer* get(PeerCommunication* p)
		{
			for (auto& peer : data)
				if (peer.comm == p)
					return &peer;

			return nullptr;
		}

	private:
		std::vector<ActivePeer>& data;
		std::mutex& mutex;

		LockedPeers& operator=(const LockedPeers& temp_obj) = delete;
	};

	class DownloaderClient
	{
	public:
		virtual bool wantsToDownload() = 0;
		virtual bool isMissingPiece(uint32_t idx) = 0;
		virtual void storePieceBlock(const PieceBlock& block) = 0;
		virtual mtt::DownloadedPiece loadUnfinishedPiece(uint32_t idx) = 0;
		virtual void pieceFinished(const mtt::DownloadedPiece& piece) = 0;

		virtual LockedPeers getPeers() = 0;
	};

	class Downloader
	{
	public:

		Downloader(const TorrentInfo& torrentInfo, DownloaderClient& client);

		std::vector<mtt::DownloadedPiece> stop();

		size_t getUnfinishedPiecesDownloadSize();
		std::map<uint32_t, uint32_t> getUnfinishedPiecesDownloadSizeMap();
		std::vector<uint32_t> getCurrentRequests() const;

		void peerAdded(ActivePeer*);
		void messageReceived(PeerCommunication*, PeerMessage&);

		void sortPieces(const std::vector<uint32_t>& availability);

		void refreshSelection(const DownloadSelection& selectedPieces, const std::vector<uint32_t>& availability);

		uint64_t downloaded = 0;

	private:

		void unchokePeer(ActivePeer*);
		void evaluateNextRequests(ActivePeer*);

		void pieceBlockReceived(PieceBlock& block, PeerCommunication* source);

		enum class PieceStatus { Ok, Invalid, Finished };
		void refreshPeerBlockRequests(std::vector<ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source);

		std::vector<uint32_t> sortedSelectedPieces;
		std::mutex sortedSelectedPiecesMutex;

		struct RequestInfo
		{
			uint32_t pieceIdx = 0;
			uint16_t blockRequestsCount = 0;
			uint16_t blocksCount = 0;
			DownloadedPiece piece;
		};
		std::vector<std::shared_ptr<RequestInfo>> requests;
		RequestInfo* getRequest(uint32_t i);
		RequestInfo* addRequest(uint32_t i);
		mutable std::mutex requestsMutex;

		struct PieceState
		{
			RequestInfo* request = nullptr;
			Priority priority = Priority(0);
			bool missing = false;
		};
		std::vector<PieceState> piecesState;

		RequestInfo* getBestNextRequest(ActivePeer*);
		bool fastCheck = true;

		uint32_t sendPieceRequests(ActivePeer*,ActivePeer::RequestedPiece*, RequestInfo*, uint32_t max);
		bool hasWantedPieces(ActivePeer*);

		uint64_t duplicatedDataSum = 0;

		const TorrentInfo& torrentInfo;
		DownloaderClient& client;

		FileLog log;
	};
}

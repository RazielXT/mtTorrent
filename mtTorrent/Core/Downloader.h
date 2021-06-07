#pragma once

#include "PeerCommunication.h"

namespace mtt
{
	struct ActivePeer
	{
		PeerCommunication* comm;

		uint32_t connectionTime = 0;
		uint32_t lastActivityTime = 0;

		uint32_t downloadSpeed = 0;
		uint32_t uploadSpeed = 0;

		uint64_t downloaded = 0;
		uint64_t uploaded = 0;

		struct RequestedPiece
		{
			uint32_t idx;
			std::vector<uint32_t> blocks;
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
		virtual bool isFinished() = 0;
		virtual bool isWantedPiece(uint32_t idx) = 0;
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
		void progressUpdated(PeerCommunication*, uint32_t idx);

		void sortPriority(const std::vector<Priority>& priority, const std::vector<uint32_t>& availability);

		void refreshSelection(std::vector<uint32_t> selectedPieces);

	private:

		void unchokePeer(ActivePeer*);
		void evaluateNextRequests(ActivePeer*);

		void pieceBlockReceived(PieceBlock& block, PeerCommunication* source);

		enum class PieceStatus { Ok, Invalid, Finished };
		void refreshPeerBlockRequests(std::vector<ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source);

		std::vector<uint32_t> selectedPieces;

		std::mutex priorityMutex;

		struct RequestInfo
		{
			uint32_t pieceIdx = 0;
			uint16_t nextBlockRequestIdx = 0;
			uint16_t blocksCount = 0;
			uint32_t lastActivityTime = 0;
			DownloadedPiece piece;
		};
		std::vector<RequestInfo> requests;
		mutable std::mutex requestsMutex;

		std::vector<uint32_t> getBestNextPieces(ActivePeer*);
		void sendPieceRequests(ActivePeer*);
		uint32_t sendPieceRequests(ActivePeer*,ActivePeer::RequestedPiece*, RequestInfo*, uint32_t max);
		bool hasWantedPieces(ActivePeer*);

		uint64_t duplicatedDataSum = 0;

		const TorrentInfo& torrentInfo;
		DownloaderClient& client;
	};
}

#pragma once

#include "Storage.h"
#include "IPeerListener.h"
#include "LogFile.h"

namespace mtt
{
	struct ActivePeer
	{
		PeerCommunication * comm;

		uint32_t connectionTime = 0;
		uint32_t lastActivityTime = 0;

		uint32_t downloadSpeed = 0;
		uint32_t uploadSpeed = 0;

		uint32_t downloaded = 0;
		uint32_t uploaded = 0;

		struct RequestedPiece
		{
			uint32_t idx;
			std::vector<uint32_t> blocks;
		};
		std::vector<RequestedPiece> requestedPieces;

		uint32_t receivedBlocks = 0;
		uint32_t invalidPieces = 0;
	};

	class Downloader
	{
	public:

		Downloader(TorrentPtr);

		enum PieceStatus {Ok, Invalid, Finished};
		PieceStatus pieceBlockReceived(PieceBlock& block);
		void removeBlockRequests(std::vector<ActivePeer>& peers, PieceBlock& block, PieceStatus status, PeerCommunication* source);
		void evaluateNextRequests(ActivePeer*);
		void unchokePeer(ActivePeer*);

		void reset();
		void sortPriorityByAvailability(std::vector<uint32_t>& availability);

		std::vector<uint32_t> getCurrentRequests();
		uint32_t getCurrentRequestsCount();

	private:

		std::vector<uint32_t> piecesPriority;
		std::mutex priorityMutex;

		struct RequestInfo
		{
			uint32_t pieceIdx = 0;
			std::shared_ptr<DownloadedPiece> piece;
			uint16_t nextBlockRequestIdx = 0;
			uint16_t blocksCount = 0;
		};
		std::vector<RequestInfo> requests;
		std::mutex requestsMutex;

		std::vector<uint32_t> getBestNextPieces(ActivePeer*);
		void sendPieceRequests(ActivePeer*);
		uint32_t sendPieceRequests(ActivePeer*,ActivePeer::RequestedPiece*, RequestInfo*, uint32_t max);
		bool pieceFinished(RequestInfo*);

		TorrentPtr torrent;

		void onFinish();
	};
}

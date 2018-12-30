#pragma once

#include "Storage.h"
#include "IPeerListener.h"

namespace mtt
{
	class Downloader : public IPeerListener
	{
	public:

		Downloader(TorrentPtr);

		void start();
		void stop();

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int code) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

	private:

		struct ActivePeer
		{
			PeerCommunication* comm;
			uint32_t finishedBlocks = 0;
			uint32_t invalidPieces = 0;

			struct RequestedPiece
			{
				uint32_t idx;
				std::vector<uint32_t> blocks;
			};
			std::vector<RequestedPiece> requestedPieces;
		};
		std::vector<ActivePeer> activePeers;
		std::mutex peersMutex;

		ActivePeer* getActivePeer(PeerCommunication*);
		void addPeer(PeerCommunication*);	//after handshake
		void evaluateNextRequests(PeerCommunication*);
		void evaluateNextRequests(ActivePeer*);	//updating progress, finishing requests
		void removePeer(PeerCommunication*);	//make room after getting better peers
		void evaluateCurrentPeers();	//request more if needed

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
		void pieceBlockReceived(PeerCommunication*,PieceBlock& block);
		bool pieceFinished(RequestInfo*);

		TorrentPtr torrent;
	};
}

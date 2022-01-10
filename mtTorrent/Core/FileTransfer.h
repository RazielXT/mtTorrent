#pragma once

#include "Storage.h"
#include "IPeerListener.h"
#include "Downloader.h"
#include "Uploader.h"
#include "utils/ScheduledTimer.h"
#include "Api/FileTransfer.h"

namespace mtt
{
	class FileTransfer : public mttApi::FileTransfer, public IPeerListener, public DownloaderClient
	{
	public:

		FileTransfer(TorrentPtr);

		void start();
		void stop();

		void refreshSelection();

		uint32_t getDownloadSpeed() const;
		uint64_t& getUploadSum();
		uint32_t getUploadSpeed() const;

		void addUnfinishedPieces(std::vector<mtt::DownloadedPiece>& pieces);
		void clearUnfinishedPieces();
		std::vector<mtt::DownloadedPiece> getUnfinishedPiecesState();

		size_t getUnfinishedPiecesDownloadSize();
		std::map<uint32_t, uint32_t> getUnfinishedPiecesDownloadSizeMap();

		std::vector<uint32_t> getCurrentRequests() const;

		std::vector<ActivePeerInfo> getPeersInfo() const;

	private:

		FileLog log;

		std::vector<uint32_t> piecesAvailability;
		std::vector<Priority> piecesPriority;

		std::vector<ActivePeer> activePeers;
		mutable std::mutex peersMutex;

		mtt::ActivePeer* getActivePeer(PeerCommunication* p);
		void addPeer(PeerCommunication*);
		void removePeer(PeerCommunication*);
		void evaluateMorePeers();

		std::shared_ptr<ScheduledTimer> refreshTimer;

		void updateMeasures();
		std::vector<std::pair<PeerCommunication*, std::pair<uint64_t, uint64_t>>> lastSpeedMeasure;
		uint32_t updateMeasuresCounter = 0;

		void evalCurrentPeers();
		void disconnectPeers(const std::vector<uint32_t>& positions);
		uint32_t peersEvalCounter = 0;

		bool isFinished() override;
		bool isWantedPiece(uint32_t idx) override;
		void storePieceBlock(const PieceBlock& block) override;
		void pieceFinished(const mtt::DownloadedPiece& piece) override;
		mtt::DownloadedPiece loadUnfinishedPiece(uint32_t idx) override;
		LockedPeers getPeers() override;

		std::mutex unsavedPieceBlocksMutex;
		size_t unsavedPieceBlocksMaxSize = 0;
		std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>> unsavedPieceBlocks;
		Status saveUnsavedPieceBlocks(const std::vector<std::pair<PieceBlockInfo, std::shared_ptr<DataBuffer>>>& blocks);
		void finishUnsavedPieceBlocks();

		std::vector<std::shared_ptr<DataBuffer>> freeDataBuffers;
		std::shared_ptr<DataBuffer> getDataBuffer();
		void returnDataBuffer(std::shared_ptr<DataBuffer>);

		std::vector<uint32_t> freshPieces;
		std::vector<mtt::DownloadedPiece> unFinishedPieces;

		Downloader downloader;
		std::shared_ptr<Uploader> uploader;

		TorrentPtr torrent;

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int code) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*, uint32_t) override;
	};
}

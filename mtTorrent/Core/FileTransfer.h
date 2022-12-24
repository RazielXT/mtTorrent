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

		FileTransfer(Torrent&);

		void start();
		void stop();

		void refreshSelection();

		uint64_t& getDownloadSum();
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

		std::vector<ActivePeer> activePeers;
		mutable std::mutex peersMutex;

		mtt::ActivePeer* getActivePeer(PeerCommunication* p);
		void addPeer(PeerCommunication*);
		void removePeer(PeerCommunication*);
		void evaluateMorePeers();

		std::shared_ptr<ScheduledTimer> refreshTimer;

		void updateMeasures();
		std::vector<std::pair<PeerCommunication*, std::pair<uint64_t, uint64_t>>> lastSpeedMeasure;
		int32_t updateMeasuresCounter = 0;

		void evalCurrentPeers();
		void disconnectPeers(const std::vector<uint32_t>& positions);
		uint32_t peersEvalCounter = 0;

		bool wantsToDownload() override;
		bool isMissingPiece(uint32_t idx) override;
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

		std::mutex unFinishedPiecesMutex;
		std::vector<mtt::DownloadedPiece> unFinishedPieces;

		Downloader downloader;
		std::shared_ptr<Uploader> uploader;

		Torrent& torrent;

		void handshakeFinished(PeerCommunication*) override;
		void connectionClosed(PeerCommunication*, int code) override;
		void messageReceived(PeerCommunication*, PeerMessage&) override;
		void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) override;
		void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView& data) override;
	};
}

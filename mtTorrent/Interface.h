#pragma once
#include <string>
#include <vector>
#include "Network.h"

extern int gcount;

#define MT_NAME "mtTorrent 0.2"
#define MT_HASH_NAME "MT-0-2-"

extern std::string getTimestamp();

namespace Checkpoint
{
	extern bool hit(int id, int msPeriod);
};

namespace Torrent
{
	struct File
	{
		int id;
		std::vector<std::string> path;
		size_t size;
		size_t startPieceIndex;
		size_t startPiecePos;
		size_t endPieceIndex;
		size_t endPiecePos;
	};

	struct PieceInfo
	{
		char hash[20];
	};

	struct TorrentInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		DataBuffer infoHash;

		std::vector<PieceInfo> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string directory;
	};

	namespace StorageType 
	{
		enum StorageEnum { File, Memory };
	}	

	struct SelectedFile
	{
		File file;
		StorageType::StorageEnum storageType;
	};

	struct DownloadSelection
	{
		std::vector<SelectedFile> files;
	};

	struct PieceBlockInfo
	{
		uint32_t index;
		uint32_t begin;
		uint32_t length;
	};

	struct PieceBlock
	{
		PieceBlockInfo info;
		DataBuffer data;
	};

	struct PieceDownloadInfo
	{
		std::vector<PieceBlockInfo> blocksLeft;
		size_t blocksCount = 0;
		char* hash;
	};

	struct DownloadedPiece
	{
		DataBuffer data;
		size_t index;
		size_t dataSize;

		void reset(size_t maxPieceSize);
		void addBlock(PieceBlock& block);
		size_t receivedBlocks = 0;
	};

	struct PiecesProgress
	{
		void init(size_t size);
		size_t piecesCount = 0;

		bool finished();
		float getPercentage();
		float getFullPercentage();

		void setSelection(std::vector<File>& files);
		void resetAdded(PiecesProgress& parent);

		void addPiece(size_t index);
		bool hasPiece(size_t index);
		bool wantsPiece(size_t index);

		void fromBitfield(DataBuffer& bitfield);
		DataBuffer toBitfield();

	private:

		enum Progress : char {NotSelected = -1, Selected = 0, Added = 1};

		std::vector<Progress> piecesProgress;
		size_t addedPiecesCount = 0;
		size_t addedSelectedPiecesCount = 0;
		size_t selectedPiecesCount = 0;
	};

	struct PeerInfo
	{
		std::string ipStr;
		uint16_t port;
		uint32_t ip;	

		void setIp(uint32_t ip);

		inline bool operator== (const PeerInfo& r)
		{
			return ip == r.ip && port == r.port;
		}
	};

	class ProgressScheduler;

	struct ClientInfo
	{
		char hashId[20];
		uint32_t key;

		uint32_t listenPort = 80;
		uint32_t maxPeersPerRequest = 100;

		ProgressScheduler* scheduler;

		struct
		{
			boost::asio::io_service* io_service;
			//tcp::resolver* resolver;
		}
		network;
	};

	extern uint32_t generateTransaction();

}
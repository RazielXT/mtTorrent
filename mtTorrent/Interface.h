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
		uint32_t startPieceIndex;
		size_t startPiecePos;
		uint32_t endPieceIndex;
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
		size_t fullSize;
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
		uint32_t index;
		size_t dataSize;

		void reset(size_t maxPieceSize);
		void addBlock(PieceBlock& block);
		size_t receivedBlocks = 0;
	};

	struct PiecesProgress
	{
		void init(size_t size);
		void reset(PiecesProgress& parent);

		bool empty();
		float getPercentage();

		void addPiece(uint32_t index);
		bool hasPiece(uint32_t index);
		void removePiece(uint32_t index);

		void fromSelection(std::vector<File>& files);
		void fromBitfield(DataBuffer& bitfield, size_t piecesCount);
		DataBuffer toBitfield();

		const std::map<uint32_t, bool>& get();

	private:

		size_t piecesStartCount = 0;
		std::map<uint32_t, bool> pieces;
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

	struct AnnounceResponse
	{
		uint32_t interval;

		uint32_t leechCount;
		uint32_t seedCount;

		std::vector<PeerInfo> peers;
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
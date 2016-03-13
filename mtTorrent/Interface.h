#pragma once
#include <string>
#include <vector>
#include "Network.h"

extern int gcount;

extern std::string getTimestamp();

namespace Torrent
{
	struct File
	{
		std::vector<std::string> path;
		size_t size;
		size_t startPieceIndex;
		size_t startPiecePos;
		size_t endPieceIndex;
		size_t endPiecePos;
	};

	struct PieceObj
	{
		char hash[20];
	};

	struct TorrentInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		DataBuffer infoHash;

		std::vector<PieceObj> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string directory;
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
		std::vector<PieceBlockInfo> blocks;
	};

	struct DownloadedPiece
	{
		std::vector<PieceBlock> blocks;
		size_t index;
	};

	struct PiecesProgress
	{
		void init(size_t size);
		size_t piecesCount = 0;

		float getPercentage();
		void addPiece(size_t index);
		bool hasPiece(size_t index);

		void fromBitfield(DataBuffer& bitfield);
		DataBuffer toBitfield();

	private:

		std::vector<bool> piecesProgress;
		size_t downloadedPieces = 0;
	};

	struct PeerInfo
	{
		uint32_t ip;
		std::string ipStr;
		uint16_t port;

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
		uint32_t maxPeersPerRequest = 40;

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
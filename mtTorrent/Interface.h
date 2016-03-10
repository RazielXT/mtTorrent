#pragma once
#include <string>
#include <vector>
#include "Network.h"

extern int gcount;

namespace Torrent
{
	struct File
	{
		std::string name;
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

		std::vector<char> infoHash;

		std::vector<PieceObj> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string directory;
	};

	struct DownloadedPiece
	{
		char* data;
		size_t index;
	};

	struct PiecesProgress
	{
		size_t piecesCount = 0;

		float getPercentage();
		void addPiece(size_t index);
		bool hasPiece(size_t index);

		void fromBitfield(std::vector<char>& bitfield);
		std::vector<char> toBitfield();

	private:

		std::vector<bool> piecesProgress;
		size_t downloadedPieces = 0;
	};

	struct PeerInfo
	{
		uint32_t ip;
		std::string ipStr;

		uint16_t port;
		uint16_t index;

		inline bool operator== (const PeerInfo& r)
		{
			return ip == r.ip && port == r.port;
		}
	};

	struct ClientInfo
	{
		char hashId[20];
		uint32_t key;

		uint32_t listenPort = 80;
		uint32_t maxPeersPerRequest = 40;
	};

	extern uint32_t generateTransaction();

}
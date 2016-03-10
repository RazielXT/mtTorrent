#pragma once
#include <string>
#include <vector>
#include "Network.h"

extern int gcount;

namespace Torrent
{
	struct TorrentInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		std::vector<char> infoHash;

		struct PieceObj
		{
			char hash[20];
		};
		std::vector<PieceObj> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		struct File
		{
			std::string name;
			size_t size;
			size_t startPieceIndex;
			size_t startPiecePos;
			size_t endPieceIndex;
			size_t endPiecePos;
		};
		std::vector<File> files;
		std::string directory;
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

	struct UdpNetwork
	{
		udp::socket* socket;
		udp::resolver* resolver;	
	};

	extern uint32_t generateTransaction();

}
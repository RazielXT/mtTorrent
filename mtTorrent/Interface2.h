#pragma once
#include <string>
#include <vector>
#include "Network.h"
#include <iostream>

#define MT_NAME "mtTorrent 0.5"
#define MT_HASH_NAME "MT-0-5-"

#define DHT_LOG(x) {std::cout << x;}
//#define NETWORK_LOG(x) {std::cout << x;}
//#define PEER_LOG(x) {std::cout << x;}
#define TRACKER_LOG(x) {std::cout << x;}
//#define PARSER_LOG(x) {std::cout << x;}
#define GENERAL_INFO_LOG(x) {std::cout << x;}

#ifndef DHT_LOG
#define DHT_LOG(x){}
#endif
#ifndef NETWORK_LOG
#define NETWORK_LOG(x){}
#endif
#ifndef PEER_LOG
#define PEER_LOG(x){}
#endif
#ifndef TRACKER_LOG
#define TRACKER_LOG(x){}
#endif
#ifndef PARSER_LOG
#define PARSER_LOG(x){}
#endif
#ifndef GENERAL_INFO_LOG
#define GENERAL_INFO_LOG(x){}
#endif

namespace mtt
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
		uint8_t hash[20];
	};

	struct TorrentInfo
	{
		uint8_t hash[20];

		std::vector<PieceInfo> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string directory;
		size_t fullSize;
	};

	struct TorrentFileInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;
	};

	struct SelectedFile
	{
		File file;
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
		uint32_t index;
	};

	struct DownloadedPiece
	{
		DataBuffer data;
		uint32_t index = -1;
		size_t dataSize = 0;

		bool isValid(char* expectedHash);
		void reset(size_t maxPieceSize);
		void addBlock(PieceBlock& block);
		size_t receivedBlocks = 0;
	};

	struct Addr
	{
		Addr();
		Addr(uint8_t* buffer, bool v6);
		Addr(uint8_t* ip, uint16_t port, bool isIpv6);
		Addr(uint32_t ip, uint16_t port);

		uint8_t addrBytes[16];
		uint16_t port;
		bool ipv6;

		void set(uint8_t* ip, uint16_t port, bool isIpv6);
		void set(DataBuffer ip, uint16_t port);
		void set(uint32_t ip, uint16_t port);

		size_t parse(uint8_t* buffer, bool v6);
	};
}
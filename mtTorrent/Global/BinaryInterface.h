#pragma once

#include <vector>

namespace mtBI
{
	struct string
	{
		string();
		~string();

		void set(const std::string& str);
		char* data = nullptr;

		struct basic_allocator
		{
			void* alloc(size_t size);
			void dealloc(void* data);
		};
	private:
		basic_allocator* allocator;
	};

	enum class MessageId
	{
		Start,
		GetTorrentInfo,
		GetTorrentStateInfo,	//TorrentStateInfo
		GetPeersInfo,	//TorrentPeersInfo
		GetSourcesInfo	//SourcesInfo
	};

	struct TorrentInfo
	{
		uint32_t filesCount;
		std::vector<string> filenames;
		std::vector<size_t> filesizes;
		size_t fullsize;
		string name;
	};

	struct TorrentStateInfo
	{
		string name;
		float progress;
		size_t downloaded;
		size_t downloadSpeed;
		uint32_t foundPeers;
		uint32_t connectedPeers;
	};

	struct PeerInfo
	{
		uint8_t id[20];
		float progress;
		size_t speed;
		char source[10];
		string addr;
	};

	struct TorrentPeersInfo
	{
		uint32_t count;
		std::vector<PeerInfo> peers;
	};

	struct SourceInfo
	{
		string name;
		char status[11];
		uint32_t peers;
		uint32_t seeds;
		uint32_t nextCheck;
		uint32_t interval;
	};

	struct SourcesInfo
	{
		uint32_t count;
		std::vector<SourceInfo> sources;
	};
};

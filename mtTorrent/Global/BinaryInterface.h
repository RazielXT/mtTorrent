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
		GetTorrentStateInfo,	//int, TorrentStateInfo
		GetPeersInfo	//TorrentPeersInfo
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
};

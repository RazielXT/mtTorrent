#pragma once
#include <vector>
#include "Interface.h"
#include <mutex>
#include <deque>

namespace mtt
{
	namespace dht
	{
		struct NodeId
		{
			uint8_t data[20];

			NodeId();
			NodeId(const char* buffer);
			void copy(const char* buffer);
			bool closerThan(NodeId& r, NodeId& target);
			bool closerThanThis(NodeId& distance, NodeId& target);
			NodeId distance(NodeId& r);
			uint8_t length();
			void setMax();

			static uint8_t length(uint8_t* data);
		};

		struct NodeInfo
		{
			NodeId id;
			Addr addr;

			size_t parse(char* buffer, bool v6);
			bool operator==(const NodeInfo& r);
		};

		struct Table
		{
			void nodeResponded(uint8_t* id, Addr& addr);
			void nodeResponded(uint8_t bucketId, Addr& addr);

			void nodeNotResponded(uint8_t* id, Addr& addr);
			void nodeNotResponded(uint8_t bucketId, Addr& addr);

		private:

			const uint32_t MaxBucketNodesCount = 8;
			const uint32_t MaxBucketCacheSize = 8;
			const uint32_t MaxBucketNodeInactiveTime = 15*60;

			struct Bucket
			{
				struct Node
				{
					Addr addr;
					uint32_t lastupdate = 0;
					bool active = true;
				};

				std::vector<Node> nodes;
				std::deque<Node> cache;

				uint32_t lastupdate = 0;
				uint32_t lastcacheupdate = 0;

				Node* find(Addr& node);
				Node* findCache(Addr& node);
			};

			std::array<Bucket, 160> buckets;
			std::mutex tableMutex;
		};
	}
}
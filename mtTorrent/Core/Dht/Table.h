#pragma once
#include <vector>
#include "Node.h"
#include <mutex>
#include <deque>

namespace mtt
{
	namespace dht
	{
		struct Table
		{
			std::vector<Addr> getClosestNodes(uint8_t* id, bool ipv6);

			void nodeResponded(uint8_t* id, Addr& addr);
			void nodeResponded(uint8_t bucketId, Addr& addr);

			void nodeNotResponded(uint8_t* id, Addr& addr);
			void nodeNotResponded(uint8_t bucketId, Addr& addr);

			uint8_t getBucketId(uint8_t* id);

			bool empty = true;

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

			std::mutex tableMutex;
			std::array<Bucket, 160> buckets;
		};
	}
}
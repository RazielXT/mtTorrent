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
			std::vector<NodeInfo> getClosestNodes(const uint8_t* id);

			void nodeResponded(NodeInfo& node);
			void nodeResponded(uint8_t bucketId, NodeInfo& node);

			void nodeNotResponded(NodeInfo& node);
			void nodeNotResponded(uint8_t bucketId, NodeInfo& node);

			uint8_t getBucketId(const uint8_t* id);

			bool empty();

			std::string save();
			uint32_t load(std::string&);

			std::vector<NodeInfo> getInactiveNodes();

		private:

			const uint32_t MaxBucketNodesCount = 8;
			const uint32_t MaxBucketCacheSize = 8;
			const uint32_t MaxBucketNodeInactiveTime = 15*60;
			const uint32_t MaxBucketFreshNodeInactiveTime = 0;

			struct Bucket
			{
				struct Node
				{
					NodeInfo info;
					uint32_t lastupdate = 0;
					bool active = true;
				};

				std::vector<Node> nodes;
				std::deque<Node> cache;

				uint32_t lastupdate = 0;
				uint32_t lastcacheupdate = 0;

				Node* find(NodeInfo& node);
				Node* findCache(NodeInfo& node);
			};

			std::mutex tableMutex;
			std::array<Bucket, 160> buckets;
		};

		bool isValidNode(const uint8_t* hash);
	}
}
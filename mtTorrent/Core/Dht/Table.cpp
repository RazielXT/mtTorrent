#include "Dht/Table.h"
#include "Configuration.h"

using namespace mtt::dht;

std::vector<Addr> mtt::dht::Table::getClosestNodes(uint8_t* id, bool ipv6)
{
	auto i = getBucketId(id);
	std::vector<Addr> out;

	std::lock_guard<std::mutex> guard(tableMutex);

	while (i > 0 && out.size() < 8)
	{
		auto& b = buckets[i];

		for (auto& n : b.nodes)
		{
			if (n.addr.ipv6 == ipv6 && out.size() < 8)
				out.push_back(n.addr);
		}

		i--;
	}

	return out;
}

void mtt::dht::Table::nodeResponded(uint8_t* id, Addr& addr)
{
	auto i = getBucketId(id);

	nodeResponded(i, addr);
}

void mtt::dht::Table::nodeResponded(uint8_t bucketId, Addr& addr)
{
	std::lock_guard<std::mutex> guard(tableMutex);

	empty = false;

	auto& bucket = buckets[bucketId];
	uint32_t time = (uint32_t)::time(0);

	auto n = bucket.find(addr);
	if (!n)
	{
		n = bucket.findCache(addr);
		if (!n)
		{
			Bucket::Node node;
			node.addr = addr;
			node.lastupdate = time;

			if (bucket.nodes.size() < MaxBucketNodesCount)
			{
				bucket.nodes.push_back(node);
				bucket.lastupdate = time;
			}
			else
			{
				node.active = false;

				if(bucket.cache.size() < MaxBucketCacheSize)
					bucket.cache.emplace_back(node);
				else
					bucket.cache.back() = node;

				bucket.lastcacheupdate = time;
			}
		}
		else
			bucket.lastcacheupdate = n->lastupdate = time;
	}
	else
	{
		bucket.lastupdate = n->lastupdate = time;
		n->active = true;
	}
}

void mtt::dht::Table::nodeNotResponded(uint8_t* id, Addr& addr)
{
	auto i = getBucketId(id);

	nodeNotResponded(i, addr);
}

void mtt::dht::Table::nodeNotResponded(uint8_t bucketId, Addr& addr)
{
	std::lock_guard<std::mutex> guard(tableMutex);

	auto& bucket = buckets[bucketId];

	for (auto it = bucket.nodes.begin(); it != bucket.nodes.end(); it++)
	{
		if (it->addr == addr)
		{
			if (it->active)
			{
				it->active = false;
				it->lastupdate = (uint32_t)::time(0);
			}

			if (bucket.lastupdate > it->lastupdate + MaxBucketNodeInactiveTime)
			{
				bucket.nodes.erase(it);

				if (!bucket.cache.empty())
				{
					bucket.nodes.push_back(bucket.cache.front());
					bucket.cache.pop_front();
				}
			}

			break;
		}
	}
}

mtt::dht::Table::Bucket::Node* mtt::dht::Table::Bucket::find(Addr& node)
{
	for (auto& n : nodes)
		if (n.addr == node)
			return &n;

	return nullptr;
}

mtt::dht::Table::Bucket::Node* mtt::dht::Table::Bucket::findCache(Addr& node)
{
	for (auto& n : cache)
		if (n.addr == node)
			return &n;

	return nullptr;
}

uint8_t mtt::dht::Table::getBucketId(uint8_t* id)
{
	return NodeId::distance(mtt::config::internal.hashId, id).length();
}

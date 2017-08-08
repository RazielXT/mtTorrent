#include "Dht/Table.h"
#include "Configuration.h"

using namespace mtt::dht;

NodeId::NodeId()
{
}

NodeId::NodeId(const char* buffer)
{
	copy(buffer);
}

void NodeId::copy(const char* buffer)
{
	memcpy(data, buffer, 20);
}

bool mtt::dht::NodeId::closerThanThis(NodeId& maxDistance, NodeId& target)
{
	auto dist = distance(target);

	for (int i = 0; i < 20; i++)
	{
		if (dist.data[i] <= maxDistance.data[i])
			return true;
		if (dist.data[i] > maxDistance.data[i])
			return false;
	}

	return false;
}

bool mtt::dht::NodeId::closerThan(NodeId& r, NodeId& target)
{
	auto otherDist = r.distance(target);

	return closerThanThis(otherDist, target);
}

NodeId mtt::dht::NodeId::distance(NodeId& r)
{
	return NodeId::distance(data, r.data);
}

uint8_t mtt::dht::NodeId::length()
{
	return NodeId::length(data);
}

void mtt::dht::NodeId::setMax()
{
	memset(data, 0xFF, 20);
}

NodeId mtt::dht::NodeId::distance(uint8_t* l, uint8_t* r)
{
	NodeId out;

	for (int i = 0; i < 20; i++)
	{
		out.data[i] = l[i] ^ r[i];
	}

	return out;
}

uint8_t mtt::dht::NodeId::length(uint8_t* data)
{
	for (int i = 0; i < 20; i++)
	{
		if (data[i])
		{
			uint8_t d = data[i];
			uint8_t l = (19 - i) * 8;

			while (d >>= 1)
				l++;

			return l;
		}
	}

	return 0;
}

size_t NodeInfo::parse(char* buffer, bool v6)
{
	id.copy(buffer);
	buffer += 20;

	return 20 + addr.parse((uint8_t*)buffer, v6);
}

bool mtt::dht::NodeInfo::operator==(const NodeInfo& r)
{
	return memcmp(id.data, r.id.data, 20) == 0;
}

std::vector<Addr> mtt::dht::Table::getClosestNodes(uint8_t* id, bool ipv6)
{
	auto i = getBucketId(id);
	std::vector<Addr> out;

	std::lock_guard<std::mutex> guard(tableMutex);

	while (i < 160 && out.size() < 8)
	{
		auto& b = buckets[i];

		for (auto& n : b.nodes)
		{
			if (n.addr.ipv6 == ipv6 && out.size() < 8)
				out.push_back(n.addr);
		}

		i++;
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

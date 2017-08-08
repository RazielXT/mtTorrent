#include "Dht/Table.h"

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
	NodeId out;

	for (int i = 0; i < 20; i++)
	{
		out.data[i] = data[i] ^ r.data[i];
	}

	return out;
}

uint8_t mtt::dht::NodeId::length()
{
	return NodeId::length(data);
}

void mtt::dht::NodeId::setMax()
{
	memset(data, 0xFF, 20);
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

void mtt::dht::Table::nodeResponded(uint8_t* id, Addr& addr)
{
	auto i = NodeId::length(id);

	auto& bucket = buckets[i];

	if (true)
	{
		Bucket::Node n;
		n.addr = addr;

		if (bucket.nodes.size() < 8)
			bucket.nodes.push_back(n);
		else
			bucket.nodes[0] = n;

		bucket.lastupdate = (uint32_t)::time(0);
	}
}

void mtt::dht::Table::nodeNotResponded(uint8_t* id, Addr& addr)
{
	auto i = NodeId::length(id);

	auto& bucket = buckets[i];

	for (auto it = bucket.nodes.begin(); it != bucket.nodes.end(); it++)
	{
		if (it->addr == addr)
		{
			if ((bucket.lastupdate + 15 * 60) < (uint32_t)::time(0))
				bucket.nodes.erase(it);

			break;
		}
	}
}

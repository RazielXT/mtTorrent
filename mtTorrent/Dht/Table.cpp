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

void mtt::dht::NodeId::setMax()
{
	memset(data, 0xFF, 20);
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

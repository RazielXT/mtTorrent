#include "Dht/Node.h"
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

bool mtt::dht::NodeId::closerToNodeThan(NodeId& maxDistance, NodeId& target)
{
	auto dist = distance(target);
	auto nl = dist.length();

	if(nl != 0)
		for (int i = 0; i < 20; i++)
		{
			if (dist.data[i] != maxDistance.data[i])
				return dist.data[i] < maxDistance.data[i];
		}

	return false;
}

bool mtt::dht::NodeId::closerThan(NodeId& other, NodeId& target)
{
	auto otherDist = other.distance(target);

	return closerToNodeThan(otherDist, target);
}

NodeId mtt::dht::NodeId::distance(NodeId& r)
{
	return NodeId::distance(data, r.data);
}

uint8_t mtt::dht::NodeId::length()
{
	return NodeId::length(data);
}

bool mtt::dht::NodeId::operator==(const uint8_t* r)
{
	return memcmp(data, r, 20) == 0;
}

bool mtt::dht::NodeId::operator==(const NodeId& r)
{
	return memcmp(data, r.data, 20) == 0;
}

void mtt::dht::NodeId::setMax()
{
	memset(data, 0xFF, 20);
}

NodeId mtt::dht::NodeId::distance(const uint8_t* l, const uint8_t* r)
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

int NodeInfo::parse(const char* buffer, bool v6)
{
	id.copy(buffer);
	buffer += 20;

	return 20 + addr.parse((uint8_t*)buffer, v6);
}

bool mtt::dht::NodeInfo::operator==(const NodeInfo& r)
{
	return memcmp(id.data, r.id.data, 20) == 0;
}

bool mtt::dht::operator<(const NodeId& l, const NodeId& r)
{
	for (int i = 0; i < 20; i++)
	{
		if (l.data[i] != r.data[i])
			return l.data[i] < r.data[i];
	}

	return false;
}

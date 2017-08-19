#include "Dht/Table.h"
#include "Configuration.h"

using namespace mtt::dht;

std::vector<Addr> mtt::dht::Table::getClosestNodes(uint8_t* id)
{
	auto startId = getBucketId(id);

	std::vector<Addr> out;

	std::lock_guard<std::mutex> guard(tableMutex);

	auto i = startId;

	int v6Counter = 0;
	int v4Counter = 0;
	size_t maxNodesCount = 16;

	while (i > 0 && out.size() < maxNodesCount)
	{
		auto& b = buckets[i];

		for (auto& n : b.nodes)
		{
			if (n.active && out.size() < maxNodesCount)
			{
				if (n.addr.ipv6 && v6Counter++ < 8)
					out.push_back(n.addr);
				else if (!n.addr.ipv6 && v4Counter++ < 8)
					out.push_back(n.addr);
			}
		}

		i--;
	}

	i = startId + 1;
	while (i < 160 && out.size() < maxNodesCount)
	{
		auto& b = buckets[i];

		for (auto& n : b.nodes)
		{
			if (n.active && out.size() < maxNodesCount)
			{
				if (n.addr.ipv6 && v6Counter++ < 8)
					out.push_back(n.addr);
				else if (!n.addr.ipv6 && v4Counter++ < 8)
					out.push_back(n.addr);
			}
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

	uint32_t i = 0;
	for (auto it = bucket.nodes.begin(); it != bucket.nodes.end(); it++)
	{
		if (it->addr == addr)
		{
			if (it->active)
			{
				it->active = false;
				it->lastupdate = (uint32_t)::time(0);
			}

			//last 2 nodes kept fresh
			uint32_t MaxNodeOfflineTime = i > 5 ? MaxBucketFreshNodeInactiveTime : MaxBucketNodeInactiveTime;

			if (bucket.lastupdate > it->lastupdate + MaxNodeOfflineTime)
			{
				bucket.nodes.erase(it);

				if (!bucket.cache.empty())
				{
					bucket.nodes.emplace_back(bucket.cache.front());
					bucket.cache.pop_front();
				}
			}

			break;
		}

		i++;
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

bool mtt::dht::Table::empty()
{
	std::mutex tableMutex;

	for (auto it = buckets.rbegin(); it != buckets.rend(); it++)
	{
		if (!it->nodes.empty())
			return false;
	}

	return true;
}

std::string mtt::dht::Table::save()
{
	std::mutex tableMutex;

	std::string state;
	state += "[buckets]\n";

	uint8_t idx = 0;
	for (auto b : buckets)
	{
		if (!b.nodes.empty())
		{
			state += std::to_string((int)idx) + "\n[nodes]\n";

			for (auto& n : b.nodes)
			{
				state.append((char*)n.addr.addrBytes, 16);
				state.append((char*)&n.addr.port, sizeof(uint16_t));
				state.append((char*)&n.addr.ipv6, sizeof(bool));
			}

			state += "[nodes\\]\n";
		}

		idx++;
	}

	state += "[buckets\\]\n";

	return state;
}

uint32_t mtt::dht::Table::load(std::string& settings)
{
	std::lock_guard<std::mutex> guard(tableMutex);

	if (settings.compare(0, 10, "[buckets]\n") != 0)
		return 0;

	if (settings.compare(settings.length() - 11, 11, "[buckets\\]\n") != 0)
		return 0;

	size_t pos = 10;
	size_t posEnd = settings.length() - 11;
	uint32_t counter = 0;

	while (pos < posEnd)
	{
		auto line = settings.substr(pos, settings.find_first_of('\n', pos) - pos);
		pos += line.length() + 1;

		auto id = std::stoi(line);

		if (settings.compare(pos, 8, "[nodes]\n") != 0)
			return counter;

		pos += 8;

		auto endNodes = settings.find("[nodes\\]\n", pos);
		if (endNodes == std::string::npos)
			return counter;

		auto nodesLine = settings.substr(pos, endNodes - pos);
		pos = endNodes + 9;

		Bucket& b = buckets[id];

		size_t nodesPos = 0;
		while (nodesPos < nodesLine.length())
		{
			Bucket::Node node;

			auto& l = nodesLine.substr(nodesPos, 19);
			memcpy(node.addr.addrBytes, l.data(), 16);
			memcpy(&node.addr.port, l.data() + 16, 2);
			memcpy(&node.addr.ipv6, l.data() + 18, 1);
			b.nodes.push_back(node);

			counter++;
			nodesPos += 19;
		}
	}

	return counter;
}

std::vector<mtt::dht::Table::BucketNode> mtt::dht::Table::getInactiveNodes()
{
	std::vector<mtt::dht::Table::BucketNode> out;

	std::lock_guard<std::mutex> guard(tableMutex);

	uint32_t now = (uint32_t)::time(0);
	uint8_t id = 0;
	for (auto&b : buckets)
	{
		for (auto& n : b.nodes)
		{
			if (!n.active || n.lastupdate + MaxBucketNodeInactiveTime < now)
			{
				out.emplace_back(BucketNode{id, n.addr});
			}
		}

		id++;
	}

	return out;
}

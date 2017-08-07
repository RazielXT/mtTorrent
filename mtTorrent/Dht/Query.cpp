#include "Dht/Query.h"
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "Configuration.h"

#define DHT_LOG(x) WRITE_LOG("DHT: " << x)

using namespace mtt::dht;

void mergeClosestNodes(std::vector<NodeInfo>& to, std::vector<NodeInfo>& from, std::vector<NodeInfo>& blacklist, uint8_t maxSize, NodeId& minDistance, NodeId& target)
{
	for (auto& n : from)
	{
		if (std::find(to.begin(), to.end(), n) != to.end())
			continue;

		if (std::find(blacklist.begin(), blacklist.end(), n) != blacklist.end())
			continue;

		if (n.id.closerThanThis(minDistance, target))
		{
			if (to.size() < maxSize)
			{
				to.push_back(n);
			}
			else
			{
				size_t replaceIdx = -1;
				uint8_t worstDistance = 0;
				auto nd = n.id.distance(target);

				for (size_t i = 0; i < to.size(); i++)
				{
					if (!to[i].id.closerThanThis(nd, target))
					{
						auto dist = to[i].id.distance(target).length();

						if (dist > worstDistance)
						{
							worstDistance = dist;
							replaceIdx = i;
						}
					}
				}

				if (replaceIdx != -1)
				{
					to[replaceIdx] = n;
				}
			}
		}
	}
}

NodeId getShortestDistance(std::vector<NodeInfo>& from, NodeId& target)
{
	NodeId id = from[0].id;

	for (size_t i = 1; i < from.size(); i++)
	{
		if (from[i].id.closerThan(id, target))
			id = from[i].id;
	}

	id = id.distance(target);

	return id;
}

GetPeersResponse Query::parseGetPeersResponse(DataBuffer& message)
{
	GetPeersResponse response;

	if (!message.empty())
	{
		BencodeParser parser;
		parser.parse(message);

		if (parser.parsedData.isMap())
		{
			if (auto tr = parser.parsedData.getTxtItem("t"))
			{
				if (!tr->empty())
					response.transaction = *reinterpret_cast<const uint16_t*>(tr->data());
			}

			if (auto resp = parser.parsedData.getDictItem("r"))
			{
				auto nodes = resp->find("nodes");
				if (nodes != resp->end() && nodes->second.type == mtt::BencodeParser::Object::Text)
				{
					auto& receivedNodes = response.nodes;
					auto& data = nodes->second.txt;

					for (size_t pos = 0; pos < data.size();)
					{
						NodeInfo info;
						pos += info.parse(&data[pos], false);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				nodes = resp->find("nodes6");
				if (nodes != resp->end() && nodes->second.type == mtt::BencodeParser::Object::Text)
				{
					auto& receivedNodes = response.nodes;
					auto& data = nodes->second.txt;

					for (size_t pos = 0; pos < data.size();)
					{
						NodeInfo info;
						pos += info.parse(&data[pos], true);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				auto values = resp->find("values");

				if (values != resp->end() && values->second.isList())
				{
					for (auto& v : *values->second.l)
					{
						response.values.emplace_back(Addr((uint8_t*)&v.txt[0], v.txt.length() >= 18));
					}
				}

				auto token = resp->find("token");
				if (token != resp->end())
				{
					response.token = token->second.txt;
				}

				auto id = resp->find("id");
				if (id != resp->end() && id->second.txt.length() == 20)
				{
					memcpy(response.id, id->second.txt.data(), 20);
				}
			}

			auto eresp = parser.parsedData.getListItem("e");

			if (eresp && !eresp->empty())
			{
				response.result = eresp->at(0).i;
			}
		}
	}

	return response;
}

void mtt::dht::Query::onGetPeersResponse(DataBuffer* data, PackedUdpRequest* source, uint16_t transactionId)
{
	if (data)
	{
		auto resp = parseGetPeersResponse(*data);

		if (resp.transaction == transactionId)
		{
			if (!resp.values.empty())
			{
				foundCount += dhtListener->onFoundPeers(targetIdNode.data, resp.values);
			}

			if (!resp.nodes.empty())
			{
				std::lock_guard<std::mutex> guard(nodesMutex);

				mergeClosestNodes(receivedNodes, resp.nodes, usedNodes, MaxCachedNodes, minDistance, targetIdNode);

				if (!receivedNodes.empty())
				{
					auto newMinDistance = getShortestDistance(receivedNodes, targetIdNode);

					if (newMinDistance.length() < minDistance.length())
					{
						minDistance = newMinDistance;
						DHT_LOG("min distance " << (int)minDistance.length());
					}
				}
			}
		}
		else
		{
			DHT_LOG("invalid get peers response transaction id, want " << transactionId << " have " << resp.transaction);
		}
	}

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto it = requests.begin(); it != requests.end(); it++)
		{
			if ((*it).get() == source)
			{
				(*it)->clear();
				requests.erase(it);
				break;
			}
		}

		if (foundCount < MaxReturnedValues)
		{
			std::lock_guard<std::mutex> guard(nodesMutex);

			while (!receivedNodes.empty() && requests.size() < MaxSimultaneousConnections)
			{
				NodeInfo next = receivedNodes.front();
				usedNodes.push_back(next);
				receivedNodes.erase(receivedNodes.begin());

				auto tr = createTransactionId();
				auto dataReq = createGetPeersRequest(targetIdNode.data, false, tr);
				auto req = SendAsyncUdp(next.addr, dataReq, *serviceIo, std::bind(&Query::onGetPeersResponse, this, std::placeholders::_1, std::placeholders::_2, tr));
				requests.push_back(req);
			}

			if (receivedNodes.empty() && requests.empty())
				dhtListener->findingPeersFinished(targetIdNode.data, foundCount);
		}
		else
			dhtListener->findingPeersFinished(targetIdNode.data, foundCount);
	}
}

uint16_t mtt::dht::Query::createTransactionId()
{
	static uint16_t adder = 900;
	adder += 100;
	return adder;
}

void mtt::dht::Query::start()
{
	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	auto tr = createTransactionId();
	auto dataReq = Query::createGetPeersRequest(targetIdNode.data, true, tr);

	std::lock_guard<std::mutex> guard(requestsMutex);
	auto req = SendAsyncUdp(Addr({ 14,200,179,207 }, 63299), dataReq, *serviceIo, std::bind(&Query::onGetPeersResponse, this, std::placeholders::_1, std::placeholders::_2, tr));
	//auto req = SendAsyncUdp(dhtRoot, dhtRootPort, true, dataReq, service.io, std::bind(&Communication::Query::onGetPeersResponse, &query, std::placeholders::_1, std::placeholders::_2));
	requests.push_back(req);
}

void mtt::dht::Query::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requests.clear();
	MaxReturnedValues = 0;
}

mtt::dht::Query::~Query()
{
	stop();
}

DataBuffer mtt::dht::Query::createGetPeersRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
{
	PacketBuilder packet(128);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal.hashId, 20);
	packet.add("9:info_hash20:", 14);
	packet.add(hash, 20);

	if (bothProtocols)
		packet.add("4:wantl2:n42:n6e", 16);

	const char* clientId = "mt02";

	packet.add("e1:q9:get_peers1:v4:", 20);
	packet.add(clientId, 4);
	packet.add("1:t2:", 5);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	return packet.getBuffer();
}

#include "Dht/Query.h"
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "Configuration.h"

#define DHT_LOG(x) WRITE_LOG("DHT: " << x)

using namespace mtt::dht;

static uint16_t createTransactionId()
{
	static uint16_t adder = 900;
	adder += 100;
	return adder;
}

static void mergeClosestNodes(std::vector<NodeInfo>& to, std::vector<NodeInfo>& from, std::vector<NodeInfo>& blacklist, uint8_t maxSize, NodeId& minDistance, NodeId& target)
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

static void mergeClosestNodes(std::vector<NodeInfo>& to, std::vector<NodeInfo>& from, uint8_t maxSize, NodeId& minDistance, NodeId& target)
{
	std::vector<NodeInfo> blacklist;
	mergeClosestNodes(to, from, blacklist, maxSize, minDistance, target);
}

static NodeId getShortestDistance(std::vector<NodeInfo>& from, NodeId& target)
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

mtt::dht::Query::GenericQuery::~GenericQuery()
{
	stop();
}

void mtt::dht::Query::GenericQuery::start(uint8_t* hash, Table* t, DhtListener* listener, boost::asio::io_service* io)
{
	table = t;
	dhtListener = listener;
	serviceIo = io;
	targetId.copy((char*)hash);
	minDistance.setMax();

	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	RequestInfo r = { NodeInfo(), createTransactionId() };
	auto dataReq = createRequest(targetId.data, true, r.transactionId);

	std::lock_guard<std::mutex> guard(requestsMutex);
	auto req = SendAsyncUdp(Addr({ 14,200,179,207 }, 63299), dataReq, *serviceIo, std::bind(&GenericQuery::onResponse, this, std::placeholders::_1, std::placeholders::_2, r));
	//auto req = SendAsyncUdp(dhtRoot, dhtRootPort, true, dataReq, service.io, std::bind(&Communication::Query::onGetPeersResponse, &query, std::placeholders::_1, std::placeholders::_2));
	requests.push_back(req);
}

void mtt::dht::Query::GenericQuery::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto r : requests)
	{
		r->clear();
	}

	requests.clear();
	MaxSimultaneousRequests = 0;
}

GetPeersResponse Query::FindPeers::parseGetPeersResponse(DataBuffer& message)
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

void Query::FindPeers::onResponse(DataBuffer* data, PackedUdpRequest* source, RequestInfo request)
{
	if (data)
	{
		auto resp = parseGetPeersResponse(*data);

		if (resp.transaction == request.transactionId)
		{
			if (!resp.values.empty())
			{
				foundCount += dhtListener->onFoundPeers(targetId.data, resp.values);
			}

			if (!resp.nodes.empty())
			{
				std::lock_guard<std::mutex> guard(nodesMutex);

				mergeClosestNodes(receivedNodes, resp.nodes, usedNodes, MaxCachedNodes, minDistance, targetId);

				if (!receivedNodes.empty())
				{
					auto newMinDistance = getShortestDistance(receivedNodes, targetId);

					if (newMinDistance.length() < minDistance.length())
					{
						minDistance = newMinDistance;
						DHT_LOG("min distance " << (int)minDistance.length());
					}
				}
			}

			table->nodeResponded(resp.id, request.node.addr);
		}
		else
		{
			DHT_LOG("invalid get peers response transaction id, want " << request.transactionId << " have " << resp.transaction);
		}
	}
	else
		table->nodeResponded(request.node.id.data, request.node.addr);

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

			while (!receivedNodes.empty() && requests.size() < MaxSimultaneousRequests)
			{
				NodeInfo next = receivedNodes.front();
				usedNodes.push_back(next);
				receivedNodes.erase(receivedNodes.begin());

				RequestInfo r = { next, createTransactionId() };
				auto dataReq = createRequest(targetId.data, false, r.transactionId);
				auto req = SendAsyncUdp(next.addr, dataReq, *serviceIo, std::bind(&FindPeers::onResponse, this, std::placeholders::_1, std::placeholders::_2, r));
				requests.push_back(req);
			}

			if (receivedNodes.empty() && requests.empty())
				dhtListener->findingPeersFinished(targetId.data, foundCount);
		}
		else
			dhtListener->findingPeersFinished(targetId.data, foundCount);
	}
}

DataBuffer Query::FindPeers::createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
{
	PacketBuilder packet(128);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal.hashId, 20);
	packet.add("9:info_hash20:", 14);
	packet.add(hash, 20);

	if (bothProtocols)
		packet.add("4:wantl2:n42:n6e", 16);

	packet.add("e1:q9:get_peers1:v4:", 20);
	packet.add(MT_NAME_SHORT, 4);
	packet.add("1:t2:", 5);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	return packet.getBuffer();
}

DataBuffer mtt::dht::Query::FindNode::createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
{
	PacketBuilder packet(128);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal.hashId, 20);
	packet.add("6:target20:", 14);
	packet.add(hash, 20);

	if (bothProtocols)
		packet.add("4:wantl2:n42:n6e", 16);

	packet.add("e1:q9:find_node1:v4:", 20);
	packet.add(MT_NAME_SHORT, 4);
	packet.add("1:t2:", 5);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	return packet.getBuffer();
}

void mtt::dht::Query::FindNode::onResponse(DataBuffer* data, PackedUdpRequest* source, RequestInfo request)
{
	if (data)
	{
		auto resp = parseFindNodeResponse(*data);

		if (resp.transaction == request.transactionId)
		{
			if (!resp.nodes.empty())
			{
				auto newMinDistance = getShortestDistance(resp.nodes, targetId);

				std::lock_guard<std::mutex> guard(nodesMutex);

				if (newMinDistance.length() < minDistance.length())
				{
					minDistance = newMinDistance;
					mergeClosestNodes(receivedNodes, resp.nodes, MaxCachedNodes, minDistance, targetId);

					DHT_LOG("min distance " << (int)minDistance.length());
				}
			}

			table->nodeResponded(resp.id, request.node.addr);
		}
		else
		{
			DHT_LOG("invalid find nodes response transaction id, want " << request.transactionId << " have " << resp.transaction);
		}
	}
	else
		table->nodeNotResponded(request.node.id.data, request.node.addr);

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

		std::lock_guard<std::mutex> guard2(nodesMutex);

		while (!receivedNodes.empty() && requests.size() < MaxSimultaneousRequests)
		{
			NodeInfo next = receivedNodes.front();
			receivedNodes.erase(receivedNodes.begin());

			RequestInfo r = { next, createTransactionId() };
			auto dataReq = createRequest(targetId.data, false, r.transactionId);
			auto req = SendAsyncUdp(next.addr, dataReq, *serviceIo, std::bind(&FindNode::onResponse, this, std::placeholders::_1, std::placeholders::_2, r));
			requests.push_back(req);
		}
	}
}

mtt::dht::FindNodeResponse mtt::dht::Query::FindNode::parseFindNodeResponse(DataBuffer& message)
{
	FindNodeResponse response;

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

#include "Dht/Query.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
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

mtt::dht::Query::DhtQuery::~DhtQuery()
{
	stop();
}

void mtt::dht::Query::DhtQuery::start(uint8_t* hash, Table* t, QueryListener* dhtListener)
{
	table = t;
	listener = dhtListener;
	targetId.copy((char*)hash);
	minDistance.setMax();

	RequestInfo r = { NodeInfo(), createTransactionId() };
	auto dataReq = createRequest(targetId.data, true, r.transactionId);

	std::lock_guard<std::mutex> guard(requestsMutex);
	
	bool used = false;

	if(!t->empty)
	{
		auto n = t->getClosestNodes(hash, true);
		auto n6 = t->getClosestNodes(hash, false);

		Addr* a = nullptr;

		if (!n.empty())
			a = &n.front();
		else if (!n6.empty())
			a = &n6.front();

		if (a)
		{
			used = true;
			sendRequest(*a, dataReq, r);
		}
	}

	if (!used)
	{
		std::string dhtRoot = "dht.transmissionbt.com";
		std::string dhtRootPort = "6881";
		//sendRequest(dhtRoot, dhtRootPort, dataReq, r);
		sendRequest(Addr({ 70,113,67,217 }, 57365), dataReq, r);
	}
}

void mtt::dht::Query::DhtQuery::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requests.clear();
	MaxSimultaneousRequests = 0;
}

bool mtt::dht::Query::DhtQuery::finished()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	return requests.empty() && receivedNodes.empty();
}

GetPeersResponse mtt::dht::Query::FindPeers::parseGetPeersResponse(DataBuffer& message)
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

bool mtt::dht::Query::FindPeers::onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request)
{
	bool handled = false;

	if (data)
	{
		auto resp = parseGetPeersResponse(*data);

		if (resp.transaction == request.transactionId)
		{
			handled = true;

			if (!resp.values.empty())
			{
				foundCount += listener->onFoundPeers(targetId.data, resp.values);
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
		table->nodeNotResponded(request.node.id.data, request.node.addr);

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto it = requests.begin(); it != requests.end(); it++)
		{
			if ((*it)->getEndpoint() == comm->getEndpoint())
			{
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
				sendRequest(next.addr, dataReq, r);
			}

			if (receivedNodes.empty() && requests.empty())
				listener->findingPeersFinished(targetId.data, foundCount);
		}
		else
			listener->findingPeersFinished(targetId.data, foundCount);
	}

	return handled;
}

DataBuffer mtt::dht::Query::FindPeers::createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
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

void mtt::dht::Query::FindNode::startOne(uint8_t* hash, Addr& addr, Table* t, QueryListener* dhtListener, boost::asio::io_service* io)
{
	table = t;
	listener = dhtListener;
	findClosest = false;

	RequestInfo r = { NodeInfo(), createTransactionId() };
	auto dataReq = createRequest(hash, true, r.transactionId);

	std::lock_guard<std::mutex> guard(requestsMutex);
	sendRequest(addr, dataReq, r);
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

bool mtt::dht::Query::FindNode::onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request)
{
	bool handled = false;

	if (data)
	{
		auto resp = parseFindNodeResponse(*data);

		if (resp.transaction == request.transactionId)
		{
			if (!resp.nodes.empty() && findClosest)
			{
				auto newMinDistance = getShortestDistance(resp.nodes, targetId);

				std::lock_guard<std::mutex> guard(nodesMutex);

				auto nexMinL = newMinDistance.length();
				auto minL = minDistance.length();

				if (nexMinL <= minL)
				{
					if (nexMinL < minL)
					{
						minDistance = newMinDistance;
						DHT_LOG("min distance " << (int)nexMinL);
					}

					mergeClosestNodes(receivedNodes, resp.nodes, usedNodes, MaxCachedNodes, minDistance, targetId);
				}

				resultCount++;
			}

			DHT_LOG("find nodes response with nodes count " << resp.nodes.size());

			table->nodeResponded(resp.id, request.node.addr);
			handled = true;
		}
		else
		{
			DHT_LOG("invalid find nodes response transaction id, want " << request.transactionId << " have " << resp.transaction);
			table->nodeNotResponded(request.node.id.data, request.node.addr);
		}
	}
	else
		table->nodeNotResponded(request.node.id.data, request.node.addr);

	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto it = requests.begin(); it != requests.end(); it++)
		{
			if ((*it) == comm)
			{
				requests.erase(it);
				break;
			}
		}

		std::lock_guard<std::mutex> guard2(nodesMutex);

		DHT_LOG("try next " << receivedNodes.empty() << ";" << requests.size() << ";" << MaxSimultaneousRequests);
		while (!receivedNodes.empty() && requests.size() < MaxSimultaneousRequests)
		{
			NodeInfo next = receivedNodes.front();
			usedNodes.push_back(next);
			receivedNodes.erase(receivedNodes.begin());

			DHT_LOG("next");

			RequestInfo r = { next, createTransactionId() };
			auto dataReq = createRequest(targetId.data, false, r.transactionId);
			sendRequest(next.addr, dataReq, r);
		}
	}

	return handled;
}

void mtt::dht::Query::FindNode::sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info)
{
	auto req = listener->sendMessage(addr, data, std::bind(&FindNode::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

void mtt::dht::Query::FindNode::sendRequest(std::string& host, std::string& port, DataBuffer& data, RequestInfo& info)
{
	auto req = listener->sendMessage(host, port, data, std::bind(&FindNode::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

void mtt::dht::Query::FindPeers::sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info)
{
	auto req = listener->sendMessage(addr, data, std::bind(&FindPeers::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

void mtt::dht::Query::FindPeers::sendRequest(std::string& host, std::string& port, DataBuffer& data, RequestInfo& info)
{
	auto req = listener->sendMessage(host, port, data, std::bind(&FindPeers::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
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

mtt::dht::Query::PingNodes::~PingNodes()
{
	stop();
}

void mtt::dht::Query::PingNodes::start(std::vector<Addr>& nodes, uint8_t bId, Table* t, QueryListener* dhtListener)
{
	uint32_t startQueriesCount = std::min(MaxSimultaneousRequests, (uint32_t)nodesLeft.size());

	if (startQueriesCount > 0)
		nodesLeft.insert(nodesLeft.begin(), nodes.begin() + startQueriesCount, nodes.end());

	table = t;
	listener = dhtListener;
	bucketId = bId;

	std::lock_guard<std::mutex> guard(requestsMutex);
	for (uint32_t i = 0; i < startQueriesCount; i++)
	{
		sendRequest(nodes[i]);
	}
}

void mtt::dht::Query::PingNodes::start(Addr& node, uint8_t bId, Table* t, QueryListener* dhtListener)
{
	table = t;
	listener = dhtListener;
	bucketId = bId;

	sendRequest(node);
}

void mtt::dht::Query::PingNodes::stop()
{
	std::lock_guard<std::mutex> guard(requestsMutex);

	requests.clear();
	MaxSimultaneousRequests = 0;
}

DataBuffer mtt::dht::Query::PingNodes::createRequest(uint16_t transactionId)
{
	PacketBuilder packet(60);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal.hashId, 20);
	packet.add("e1:q4:ping1:t2:", 15);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	return packet.getBuffer();
}

bool mtt::dht::Query::PingNodes::onResponse(UdpRequest comm, DataBuffer* data, PingInfo request)
{
	if (data)
	{
		//todo check msg
		table->nodeResponded(bucketId, request.addr);
	}
	else
		table->nodeNotResponded(bucketId, request.addr);

	std::lock_guard<std::mutex> guard(requestsMutex);

	for (auto it = requests.begin(); it != requests.end(); it++)
	{
		if ((*it) == comm)
		{
			requests.erase(it);
			break;
		}
	}

	if(!nodesLeft.empty())
	{
		sendRequest(nodesLeft.back());
		nodesLeft.resize(nodesLeft.size() - 1);
	}

	return true;
}

void mtt::dht::Query::PingNodes::sendRequest(Addr& addr)
{
	PingInfo info = { createTransactionId(), addr };
	auto dataReq = createRequest(info.transactionId);
	auto req = listener->sendMessage(addr, dataReq, std::bind(&PingNodes::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

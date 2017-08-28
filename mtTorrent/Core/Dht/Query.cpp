#include "Dht/Query.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/BencodeParser.h"

#define DHT_LOG(x) WRITE_LOG("DHT: " << x)

using namespace mtt::dht;

static uint16_t createTransactionId()
{
	static std::atomic<uint16_t> adder = 0;
	adder += 100;
	return adder;
}

static void mergeClosestNodes(std::vector<NodeInfo>& to, std::vector<NodeInfo>& from, std::vector<NodeInfo>& blacklist, uint8_t maxSize, uint8_t minDistance, NodeId& target)
{
	for (auto& n : from)
	{
		if (std::find(to.begin(), to.end(), n) != to.end())
			continue;

		if (std::find(blacklist.begin(), blacklist.end(), n) != blacklist.end())
			continue;

		if (!isValidNode(n.id.data))
			continue;

		if (n.id.distance(target).length() <= minDistance || blacklist.size() < 32)
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
					if (!to[i].id.closerToNodeThan(nd, target))
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

mtt::dht::Query::DhtQuery::DhtQuery()
{
	minDistance.setMax();
}

mtt::dht::Query::DhtQuery::~DhtQuery()
{
	stop();
}

void mtt::dht::Query::DhtQuery::start(uint8_t* hash, Table* t, DataListener* dhtListener)
{
	table = t;
	listener = dhtListener;
	targetId.copy((char*)hash);

	if (t->empty())
		Sleep(500);

	auto nodes = t->getClosestNodes(hash);

	if (!nodes.empty())
	{
		std::lock_guard<std::mutex> guard(requestsMutex);

		for (auto& n : nodes)
		{
			RequestInfo r = { n, createTransactionId(), 160 };
			auto dataReq = createRequest(targetId.data, true, r.transactionId);

			sendRequest(n.addr, dataReq, r);
		}
	}
	else
		listener->findingPeersFinished(hash, 0);
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

GetPeersResponse mtt::dht::Query::GetPeers::parseGetPeersResponse(DataBuffer& message)
{
	GetPeersResponse response;

	if (!message.empty())
	{
		BencodeParser parser;
		parser.parse(message.data(), message.size());
		auto root = parser.getRoot();

		if (root->isMap())
		{
			if (auto type = root->getTxtItem("y"))
			{
				if (*type->data != 'r')
					return response;
			}

			if (auto tr = root->getTxtItem("t"))
			{
				if (tr->size)
					response.transaction = *reinterpret_cast<const uint16_t*>(tr->data);
			}

			if (auto resp = root->getDictItem("r"))
			{
				if (auto nodes = resp->getTxtItem("nodes"))
				{
					auto& receivedNodes = response.nodes;

					for (int pos = 0; pos < nodes->size;)
					{
						NodeInfo info;
						pos += info.parse(nodes->data + pos, false);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				if (auto nodes = resp->getTxtItem("nodes6"))
				{
					auto& receivedNodes = response.nodes;

					for (int pos = 0; pos < nodes->size;)
					{
						NodeInfo info;
						pos += info.parse(nodes->data + pos, true);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				if (auto values = resp->getListItem("values"))
				{
					auto value = values->getFirstItem();
					while(value)
					{
						if(value->isText())
							response.values.emplace_back(Addr((uint8_t*)value->info.data, value->info.size >= 18));

						value = value->getNextSibling();
					}
				}

				if (auto token = resp->getTxtItem("token"))
				{
					response.token = std::string(token->data, token->size);
				}

				if (auto id = resp->getTxtItem("id"))
				{
					if (id->size == 20)
					{
						memcpy(response.id, id->data, 20);
					}
				}
			}

			if (auto eresp = root->getListItem("e"))
			{
				if (auto ecode = eresp->getFirstItem())
					response.result = ecode->getInt();
			}
		}
	}

	return response;
}

bool mtt::dht::Query::GetPeers::onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request)
{
	bool handled = false;

	if (data)
	{
		auto resp = parseGetPeersResponse(*data);

		if (resp.transaction == request.transactionId && request.node.id == resp.id)
		{
			handled = true;

			if (!resp.values.empty())
			{
				foundCount += listener->onFoundPeers(targetId.data, resp.values);
			}

			if (!resp.nodes.empty())
			{
				std::lock_guard<std::mutex> guard(nodesMutex);

				mergeClosestNodes(receivedNodes, resp.nodes, usedNodes, MaxCachedNodes, request.minDistance, targetId);

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

			if (!resp.token.empty())
				listener->announceTokenReceived(resp.id, resp.token, comm->getEndpoint());

			table->nodeResponded(request.node);
		}
		else
		{
			DHT_LOG("invalid get peers response, tid " << request.transactionId << "/" << resp.transaction);
		}
	}
	else
		table->nodeNotResponded(request.node);

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

		bool finished = false;

		{
			std::lock_guard<std::mutex> guard(nodesMutex);

			if (foundCount < MaxReturnedValues)
			{
				while (!receivedNodes.empty() && requests.size() < MaxSimultaneousRequests)
				{
					NodeInfo next = receivedNodes.front();
					usedNodes.push_back(next);
					receivedNodes.erase(receivedNodes.begin());

					RequestInfo r = { next, createTransactionId(), next.id.distance(targetId).length() };
					auto dataReq = createRequest(targetId.data, true, r.transactionId);
					sendRequest(next.addr, dataReq, r);
				}
			}
			
			finished = requests.empty();
		}

		if(finished)
			listener->findingPeersFinished(targetId.data, foundCount);
	}

	return handled;
}

DataBuffer mtt::dht::Query::GetPeers::createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
{
	PacketBuilder packet(128);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internall.hashId, 20);
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

void mtt::dht::Query::FindNode::startOne(uint8_t* hash, Addr& addr, Table* t, DataListener* dhtListener)
{
	table = t;
	listener = dhtListener;
	findClosest = false;

	NodeInfo info;
	info.addr = addr;
	RequestInfo r = { info, createTransactionId(), 160 };
	auto dataReq = createRequest(hash, true, r.transactionId);

	std::lock_guard<std::mutex> guard(requestsMutex);
	sendRequest(addr, dataReq, r);
}

DataBuffer mtt::dht::Query::FindNode::createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId)
{
	PacketBuilder packet(128);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internall.hashId, 20);
	packet.add("6:target20:", 11);
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

		DHT_LOG(comm->getName() << " find node response id " << resp.id);

		if (resp.transaction == request.transactionId && request.node.id == resp.id)
		{
			if (!resp.nodes.empty() && findClosest)
			{
				auto newMinDistance = getShortestDistance(resp.nodes, targetId);

				std::lock_guard<std::mutex> guard(nodesMutex);

				mergeClosestNodes(receivedNodes, resp.nodes, usedNodes, MaxCachedNodes, request.minDistance, targetId);

				auto nexMinL = newMinDistance.length();
				auto minL = minDistance.length();

				if (nexMinL < minL && nexMinL != 0)
				{
					minDistance = newMinDistance;
					DHT_LOG("min distance " << (int)nexMinL);
				}

				DHT_LOG("find nodes response nodes count " << resp.nodes.size() << " current distance " << (int)minL << " received distance " << (int)nexMinL << " source distance" << request.minDistance << ", nodes todo count " << receivedNodes.size());
				resultCount++;
			}

			table->nodeResponded(request.node);
			handled = true;
		}
		else
		{
			DHT_LOG("invalid find nodes response, tid " << request.transactionId << "/" << resp.transaction);
		}
	}
	else
		table->nodeNotResponded(request.node);

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

		while (!receivedNodes.empty() && requests.size() < MaxSimultaneousRequests)
		{
			NodeInfo next = receivedNodes.front();
			usedNodes.push_back(next);
			receivedNodes.erase(receivedNodes.begin());

			RequestInfo r = { next, createTransactionId(), next.id.distance(targetId).length() };
			auto dataReq = createRequest(targetId.data, true, r.transactionId);
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

void mtt::dht::Query::GetPeers::sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info)
{
	auto req = listener->sendMessage(addr, data, std::bind(&GetPeers::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

mtt::dht::FindNodeResponse mtt::dht::Query::FindNode::parseFindNodeResponse(DataBuffer& message)
{
	FindNodeResponse response;

	if (!message.empty())
	{
		BencodeParser parser;
		parser.parse(message.data(), message.size());
		auto root = parser.getRoot();

		if (root->isMap())
		{
			if (auto type = root->getTxtItem("y"))
			{
				if (*type->data != 'r')
					return response;
			}

			if (auto tr = root->getTxtItem("t"))
			{
				if (tr->size)
					response.transaction = *reinterpret_cast<const uint16_t*>(tr->data);
			}

			if (auto resp = root->getDictItem("r"))
			{
				if (auto nodes = resp->getTxtItem("nodes"))
				{
					auto& receivedNodes = response.nodes;

					for (int pos = 0; pos < nodes->size;)
					{
						NodeInfo info;
						pos += info.parse(nodes->data + pos, false);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				if (auto nodes = resp->getTxtItem("nodes6"))
				{
					auto& receivedNodes = response.nodes;

					for (int pos = 0; pos < nodes->size;)
					{
						NodeInfo info;
						pos += info.parse(nodes->data + pos, true);

						if (std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				if (auto id = resp->getTxtItem("id"))
				{
					if(id->size == 20)
						memcpy(response.id, id->data, 20);
				}
			}

			if (auto eresp = root->getListItem("e"))
			{
				if (auto ecode = eresp->getFirstItem())
					response.result = ecode->getInt();
			}
		}
	}

	return response;
}

mtt::dht::Query::PingNodes::~PingNodes()
{
	stop();
}

void mtt::dht::Query::PingNodes::start(std::vector<NodeInfo>& nodes,Table* t, DataListener* dhtListener)
{
	uint32_t startQueriesCount = std::min(MaxSimultaneousRequests, (uint32_t)nodes.size());

	if (startQueriesCount > 0)
		nodesLeft.insert(nodesLeft.begin(), nodes.begin() + startQueriesCount, nodes.end());

	table = t;
	listener = dhtListener;

	std::lock_guard<std::mutex> guard(requestsMutex);
	for (uint32_t i = 0; i < startQueriesCount; i++)
	{
		sendRequest(nodes[i], false);
	}
}

void mtt::dht::Query::PingNodes::start(Addr& addr, Table* t, DataListener* dhtListener)
{
	table = t;
	listener = dhtListener;

	sendRequest(NodeInfo{NodeId(), addr }, true);
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
	packet.add(mtt::config::internall.hashId, 20);
	packet.add("e1:q4:ping1:t2:", 15);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	return packet.getBuffer();
}

bool mtt::dht::Query::PingNodes::onResponse(UdpRequest comm, DataBuffer* data, PingInfo request)
{
	WRITE_LOG(comm->getEndpoint().address().to_string() << " Ping handling, data: " << (data ? data->size() : 0) << ", tr: " << request.transactionId);

	if (data)
	{
		auto resp = parseResponse(*data);

		if (request.unknown)
		{
			memcpy(request.node.id.data, resp.id, 20);
		}

		if (resp.transaction == request.transactionId && request.node.id == resp.id)
		{
			table->nodeResponded(request.node);
		}
	}
	else if(!request.unknown)
		table->nodeNotResponded(request.node);

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
		auto&n = nodesLeft.back();
		sendRequest(n, false);
		nodesLeft.resize(nodesLeft.size() - 1);
	}

	return true;
}

mtt::dht::PingMessage mtt::dht::Query::PingNodes::parseResponse(DataBuffer& message)
{
	PingMessage response;
	memset(response.id, 0, 20);

	if (!message.empty())
	{
		BencodeParser parser;
		parser.parse(message.data(), message.size());
		auto root = parser.getRoot();

		if (root && root->isMap())
		{
			if (auto type = root->getTxtItem("y"))
			{
				if (*type->data != 'r')
					return response;
			}

			if (auto tr = root->getTxtItem("t"))
			{
				if (tr->size)
					response.transaction = *reinterpret_cast<const uint16_t*>(tr->data);
			}

			if (auto resp = root->getDictItem("r"))
			{
				auto idNode = resp->getTxtItem("id");
				if (idNode && idNode->size == 20)
				{
					memcpy(response.id, idNode->data, 20);
				}
			}

			if (auto eresp = root->getListItem("e"))
			{
				if (auto ecode = eresp->getFirstItem())
					response.result = ecode->getInt();
			}
		}
	}

	return response;
}

void mtt::dht::Query::PingNodes::sendRequest(NodeInfo& node, bool unknown)
{
	PingInfo info = { createTransactionId(), node, unknown };
	auto dataReq = createRequest(info.transactionId);
	auto req = listener->sendMessage(node.addr, dataReq, std::bind(&PingNodes::onResponse, shared_from_this(), std::placeholders::_1, std::placeholders::_2, info));
	requests.push_back(req);
}

void mtt::dht::Query::AnnouncePeer(uint8_t* infohash, std::string& token, udp::endpoint& target, DataListener* dhtListener)
{
	PacketBuilder packet(64);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internall.hashId, 20);

	if(mtt::config::external.tcpPort == mtt::config::external.udpPort)
		packet.add("12:implied_porti1e", 18);
	packet.add("4:porti", 7);
	packet << std::to_string(mtt::config::external.udpPort);
	packet.add('e');

	packet.add("9:info_hash20:", 14);
	packet.add(infohash, 20);

	packet.add("5:token", 7);
	packet << std::to_string(token.length());
	packet.add(':');
	packet.add(token.data(), token.length());

	packet.add("e1:q13:announce_peer1:t2:", 25);
	auto transactionId = createTransactionId();
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	dhtListener->sendMessage(target, packet.getBuffer());
}

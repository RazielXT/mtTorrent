#include "Responder.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"

#define DHT_LOG(x) WRITE_GLOBAL_LOG(Dht, "REMOTE " << x)

mtt::dht::Responder::Responder(DataListener& l) : listener(l)
{
	tokenSecret[0] = tokenSecret[1] = Random::Number();
}

bool mtt::dht::Responder::handlePacket(const udp::endpoint& endpoint, const BufferView& data)
{
	if (!data.size)
		return false;

	BencodeParser parser;
	if (!parser.parse(data.data, data.size) || !parser.getRoot()->isMap())
		return false;

	auto root = parser.getRoot();
	auto msgType = root->getTxtItem("y");
	if (!msgType || !(msgType->size == 1 && *msgType->data == 'q'))
		return false;

	auto transactionId = root->getTxtItem("t");
	auto requestType = root->getTxtItem("q");
	auto requestData = root->getDictObject("a");

	if (!transactionId || !requestType || !requestData)
		return false;

	auto sourceId = requestData->getTxtItem("id");

	if (!sourceId || sourceId->size != 20)
		return false;

	PacketBuilder response(requestType->size == 9 ? 256 : 64);
	response.add("d1:t", 4);
	response << std::to_string(transactionId->size);
	response.add(':');
	response.add(transactionId->data, transactionId->size);
	response.add("1:y1:r", 6);
	response.add("1:rd2:id20:", 11);
	response.add(mtt::config::getInternal().hashId, 20);

	DHT_LOG("Request: " << std::string(requestType->data, requestType->data + requestType->size) << " from " << endpoint.address().to_string());// hexToString((const uint8_t*)sourceId->data, sourceId->size));

	if (requestType->equals("find_node", 9))
	{
		auto target = requestData->getTxtItem("target");
		if (!target || target->size != 20)
			return false;

		writeNodes(target->data, endpoint, requestData, response);
	}
	else if (requestType->equals("get_peers", 9))
	{
		auto info = requestData->getTxtItem("info_hash");
		if (!info || info->size != 20)
			return false;

		writeNodes(info->data, endpoint, requestData, response);
		writeValues(info->data, endpoint, response);

		response.add("5:token4:", 9);
		auto token = getAnnounceToken(endpoint);
		response.add((char*)&token, 4);
	}
	else if (requestType->equals("announce_peer", 13))
	{
		auto infoHash = requestData->getTxtItem("info_hash");
		auto token = requestData->getTxtItem("token");

		if (!infoHash || infoHash->size != 20 || !token || token->size != 4 || !isValidToken(*(uint32_t*)token->data, endpoint))
			return false;

		int port = requestData->getInt("port");
		bool impliedPort = requestData->getInt("implied_port") == 1;

		announcedPeer(infoHash->data, Addr(endpoint.address(), (impliedPort || !port) ? endpoint.port() : (uint16_t)port));
	}

	response.add("ee", 2);

	listener.sendMessage(endpoint, response.getBuffer());

	table->nodeResponded(NodeInfo{ sourceId->data, Addr(endpoint.address(), endpoint.port()) });

	return true;
}

bool mtt::dht::Responder::writeNodes(const char* hash, const udp::endpoint& endpoint, const mtt::BencodeParser::Object* requestData, PacketBuilder& out)
{
	bool wantV4 = endpoint.address().is_v4();
	bool wantV6 = endpoint.address().is_v6();

	if (auto want = requestData->getListObject("want"))
	{
		auto wantWhat = want->getFirstItem();
		while (wantWhat && wantWhat->isText())
		{
			if (wantWhat->info.equals("n4", 2))
				wantV4 = true;
			else if (wantWhat->info.equals("n6", 2))
				wantV6 = true;

			wantWhat = wantWhat->getNextSibling();
		}
	}

	auto found = table->getClosestNodes((const uint8_t*)hash);

	if (found.empty())
		return false;

	PacketBuilder nodes;
	PacketBuilder nodesV6;

	for (auto& n : found)
	{
		if (n.addr.ipv6 && wantV6)
		{
			nodesV6.add(n.id.data, 20);
			nodesV6.add(n.addr.addrBytes, 16);
			nodesV6.add16(n.addr.port);
		}
		else if (!n.addr.ipv6 && wantV4)
		{
			nodes.add(n.id.data, 20);
			nodes.add(n.addr.addrBytes, 4);
			nodes.add16(n.addr.port);
		}
	}

	out.add("5:nodes", 7);
	out << std::to_string(nodes.data.size() + nodesV6.data.size());
	out.add(':');

	if (!nodes.data.empty())
		out.add(nodes.data.data(), nodes.data.size());
	if (!nodesV6.data.empty())
		out.add(nodesV6.data.data(), nodesV6.data.size());
	
	return false;
}

bool mtt::dht::Responder::writeValues(const char* infoHash, const udp::endpoint& endpoint, PacketBuilder& out)
{
	auto it = values.find(*(NodeId*)infoHash);
	if (it == values.end())
		return false;

	bool wantv6 = endpoint.address().is_v6();

	uint32_t maxcount = mtt::config::getInternal().dht.maxPeerValuesResponse;
	if (wantv6)
		maxcount = uint32_t(maxcount / 3.0f);
	uint32_t count = 0;

	auto addrPrefix = wantv6 ? "18:" : "6:";
	uint32_t addrPrefixSize = wantv6 ? 3 : 2;

	out.add("6:valuesl", 9);

	for (auto& v : it->second)
	{
		if (count < maxcount && v.addr.ipv6 == wantv6)
		{
			out.add(addrPrefix, addrPrefixSize);
			out.add(v.addr.addrBytes, wantv6 ? 16 : 4);
			out.add16(v.addr.port);
			count++;
		}
	}

	if (count > 0)
		out.add('e');
	else
		out.data.resize(out.data.size() - 9);

	return count > 0;
}

void mtt::dht::Responder::announcedPeer(const char* infoHash, const Addr& peer)
{
	DHT_LOG("announcedPeer " << peer.toString() << " infoHash " << hexToString((const uint8_t*)infoHash, 20));

	NodeId id;
	id.copy(infoHash);

	StoredValue v;
	v.timestamp = mtt::CurrentTimestamp();
	v.addr = peer;

	std::lock_guard<std::mutex> guard(valuesMutex);

	auto& vals = values[id];

	auto it = vals.begin();
	for (; it != vals.end(); it++)
		if (it->addr == peer)
			break;

	if (it != vals.end())
		*it = v;
	else if (vals.size() < mtt::config::getInternal().dht.maxStoredAnnouncedPeers)
		vals.push_back(v);
}

void mtt::dht::Responder::refreshStoredValues()
{
	std::lock_guard<std::mutex> guard(valuesMutex);

	const auto oldestTime = mtt::CurrentTimestamp() - 30*60;
	for (auto it = values.begin(); it != values.end();)
	{
		auto& vals = it->second;
		for (auto it2 = vals.begin(); it2 != vals.end();)
		{
			if (it2->timestamp < oldestTime)
				it2 = vals.erase(it2);
			else
				it2++;
		}

		if (it->second.empty())
			it = values.erase(it);
		else
			it++;
	}
}

bool mtt::dht::Responder::isValidToken(uint32_t token, const udp::endpoint& e)
{
	std::lock_guard<std::mutex> guard(tokenMutex);
	
	auto addr = e.address().to_string();

	if (getAnnounceToken(addr, tokenSecret[0]) == token)
		return true;
	if (getAnnounceToken(addr, tokenSecret[1]) == token)
		return true;

	return false;
}

uint32_t mtt::dht::Responder::getAnnounceToken(const udp::endpoint& e)
{
	std::lock_guard<std::mutex> guard(tokenMutex);

	return getAnnounceToken(e.address().to_string(), tokenSecret[0]);
}

uint32_t mtt::dht::Responder::getAnnounceToken(const std::string& addr, uint32_t secret)
{
	uint32_t out = secret;
	
	for (auto& c : addr)
	{
		out += c;
	}

	return out;
}

void mtt::dht::Responder::refresh()
{
	refreshStoredValues();

	{
		std::lock_guard<std::mutex> guard(tokenMutex);

		tokenSecret[1] = tokenSecret[0];
		tokenSecret[0] = Random::Number();
	}
}

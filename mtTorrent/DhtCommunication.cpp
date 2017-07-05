#include "DhtCommunication.h"
#include "Network.h"
#include "TorrentDefines.h"
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "utils/Base32.h"
#include <future>

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
	for (int i = 0; i < 20; i++)
		data[i] = (uint8_t)buffer[i];
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

size_t NodeInfo::parse(char* buffer, bool v6)
{
	id.copy(buffer);
	buffer += 20;

	return 20 + addr.parse(buffer, v6);
}

size_t NodeAddr::parse(char* buffer, bool v6)
{
	size_t addrSize = v6 ? 16 : 4;

	if (!v6)
	{
		uint8_t ipAddr[4];
		ipAddr[0] = *reinterpret_cast<uint8_t*>(buffer);
		ipAddr[1] = *(reinterpret_cast<uint8_t*>(buffer + 1));
		ipAddr[2] = *(reinterpret_cast<uint8_t*>(buffer + 2));
		ipAddr[3] = *(reinterpret_cast<uint8_t*>(buffer + 3));

		uint32_t ip = swap32(*reinterpret_cast<uint32_t*>(&buffer));
		addrBytes.assign(ipAddr, ipAddr + 4);

		str.resize(16);
		str.resize(sprintf_s(&str[0], 16, "%d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]));
	}
	else
	{
		addrBytes.assign(buffer, buffer + addrSize);

		str.resize(40);
		sprintf_s(&str[0], 40, "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
			addrBytes[0], addrBytes[1], addrBytes[2], addrBytes[3], addrBytes[4], addrBytes[5], addrBytes[6], addrBytes[7],
			addrBytes[8], addrBytes[9], addrBytes[10], addrBytes[11], addrBytes[12], addrBytes[13], addrBytes[14], addrBytes[15]);
		str.resize(39);
	}

	buffer += addrSize;

	port = _byteswap_ushort(*reinterpret_cast<const uint16_t*>(buffer));

	return 2 + addrSize;
}

bool mtt::dht::NodeInfo::operator==(const NodeInfo& r)
{
	return memcmp(id.data, r.id.data, 20) == 0;
}

mtt::dht::NodeAddr::NodeAddr()
{

}

mtt::dht::NodeAddr::NodeAddr(char* buffer, bool v6)
{
	parse(buffer, v6);
}

bool mtt::dht::NodeAddr::isIpv6()
{
	return addrBytes.size() > 4;
}

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

GetPeersResponse Communication::parseGetPeersResponse(DataBuffer& message)
{
	GetPeersResponse response;

	if (!message.empty())
	{
		BencodeParser parser;
		parser.parse(message);

		if (parser.parsedData.isMap())
		{
			auto resp = parser.parsedData.getDictItem("r");

			if (resp)
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

						if(std::find(receivedNodes.begin(), receivedNodes.end(), info) == receivedNodes.end())
							receivedNodes.push_back(info);
					}
				}

				auto values = resp->find("values");

				if (values != resp->end() && values->second.isList())
				{
					for (auto& v : *values->second.l)
					{
						response.values.emplace_back(NodeAddr(&v.txt[0], v.txt.length() >= 18));
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

//#define ASYNC_DHT

char fromHexa(char h)
{
	if (h <= '9')
		h = h - '0';
	else
		h = h - 'a' + 10;

	return h;
}

void Communication::test()
{
	/*NodeId testdata;
	memset(testdata.data, 0, 20);
	testdata.data[2] = 8;
	auto l = testdata.length();*/

	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	bool ipv6 = true;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock_v6(io_service);
	sock_v6.open(udp::v6());

	udp::socket sock_v4(io_service);
	sock_v4.open(udp::v4());

	std::string myId(20,0);
	for (int i = 0; i < 20; i++)
	{
		myId[i] = 5 + i * 5;
	}

	std::string targetIdBase32 = "T323KFN5XLZAZZO2NDNCYX7OBMQTUV6U"; // "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);

	/*auto hexaStr = "c90bb8324012a7d515901f4b3edaf02f02a5fae9";
	for (size_t i = 0; i < 20; i++)
	{
		char f = fromHexa(hexaStr[i * 2]);
		char s = fromHexa(hexaStr[i * 2 + 1]);

		targetId[i] = (f << 4) + s;
	}*/

	const char* clientId = "mt02";
	uint16_t transactionId = 54535;

	PacketBuilder packet(104);
	packet.add("d1:ad2:id20:",12);
	packet.add(myId.data(), 20);
	packet.add("9:info_hash20:",14);
	packet.add(targetId.data(), 20);
	packet.add("e1:q9:get_peers1:v4:",20);
	packet.add(clientId,4);
	packet.add("1:t2:",5);
	packet.add(reinterpret_cast<char*>(&transactionId),2);
	packet.add("1:y1:qe",7);

	PacketBuilder ipv4packet = packet;
	ipv4packet.addAfter(targetId.data(), "4:wantl2:n42:n6e", 16);

	std::vector<NodeInfo> receivedNodes;

	try
	{	
		auto message = sendUdpRequest(ipv6 ? sock_v6 : sock_v4, resolver, packet.getBuffer(), dhtRoot, dhtRootPort, 5000, ipv6);
		auto resp = parseGetPeersResponse(message);
		std::vector<NodeInfo> nextNodes = resp.nodes;
		
	/*	std::vector<NodeInfo> nextNodes;
		NodeInfo info;
		info.addr.str = "127.0.0.1";
		info.addr.port = 56572;
		nextNodes.push_back(info);*/

		std::vector<NodeInfo> usedNodes;
		std::vector<NodeAddr> values;
		NodeId minDistance = nextNodes.front().id;
		NodeId targetIdNode(targetId.data());

		bool firstWave = false;

		while (!nextNodes.empty() && values.empty())
		{
			auto currentNodes = nextNodes;
			usedNodes.insert(usedNodes.end(), currentNodes.begin(), currentNodes.end());

			nextNodes.clear();

#ifdef ASYNC_DHT
			std::vector<std::future<GetPeersResponse>> f;
#endif

			for(size_t i = 0; i < currentNodes.size(); i++)
			{
				auto& node = currentNodes[i];

#ifdef ASYNC_DHT
				if (f.size() >= 5)
				{
					for (auto& r : f)
					{
						auto resp = r.get();
						mergeClosestNodes(nextNodes, resp.nodes, usedNodes, 64, minDistance, targetIdNode);
					}

					f.clear();
				}

				f.push_back(std::async([&]() 
				{
					GetPeersResponse resp;

					try
					{
						auto message = sendUdpRequest(node.addr.isIpv6() ? sock_v6 : sock_v4, firstWave ? packet.getBuffer() : ipv4packet.getBuffer(), node.addr.str.data(), node.addr.port, 3000);
						resp = parseGetPeersResponse(message);
					}
					catch (const std::exception&e)
					{
						DHT_LOG("DHT exception: " << e.what() << "\n");
					}

					return resp;
				}));
#else
				try
				{
					auto message = sendUdpRequest(node.addr.isIpv6() ? sock_v6 : sock_v4, firstWave ? packet.getBuffer() : ipv4packet.getBuffer(), node.addr.str.data(), node.addr.port, 3000);
					auto resp = parseGetPeersResponse(message);
					mergeClosestNodes(nextNodes, resp.nodes, usedNodes, 16, minDistance, targetIdNode);

					values.assign(resp.values.begin(), resp.values.end());
				}
				catch (const std::exception&e)
				{
					DHT_LOG("DHT exception: " << e.what() << "\n");
				}
#endif
			}

#ifdef ASYNC_DHT
			if (!f.empty())
			{
				for (auto& r : f)
				{
					auto resp = r.get();
					mergeClosestNodes(nextNodes, resp.nodes, usedNodes, 32, minDistance, targetIdNode);
				}

				f.clear();
			}
#endif

			if (!nextNodes.empty())
			{
				minDistance = getShortestDistance(nextNodes, targetIdNode);
				DHT_LOG("DHT distance: " << (int)minDistance.length() << " next nodes count: " << nextNodes.size() << "\n");
			}

			firstWave = false;
		}

		if (!values.empty())
		{
			DHT_LOG("DHT returned values count: " << values.size() << "\n");

			for (auto& addr : values)
			{
				try
				{
					auto message = sendUdpRequest(addr.isIpv6() ? sock_v6 : sock_v4, firstWave ? packet.getBuffer() : ipv4packet.getBuffer(), addr.str.data(), addr.port, 3000);
					auto resp = parseGetPeersResponse(message);
				}
				catch (const std::exception&e)
				{
					DHT_LOG("DHT exception: " << e.what() << "\n");
				}
			}
		}

	}
	catch (const std::exception&e)
	{
		DHT_LOG("DHT exception: " << e.what() << "\n");
	}
}


#include "DhtCommunication.h"
#include "Network.h"
#include "TorrentDefines.h"
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "utils/Base32.h"

struct NodeId
{
	uint8_t data[20];

	NodeId()
	{
	}

	NodeId(char* buffer)
	{
		copy(buffer);
	}

	void copy(char* buffer)
	{
		for (int i = 0; i < 20; i++)
			data[i] = (uint8_t)buffer[i];
	}

	bool shorterThan(NodeId& r)
	{
		for (int i = 0; i < 20; i++)
		{
			if (data[i] < r.data[i])
				return true;
		}

		return false;
	}
};

struct NodeInfo
{
	NodeId id;

	std::string addr;
	uint16_t port;

	void parse(char* buffer, bool v6)
	{
		id.copy(buffer);
		buffer += 20;

		size_t addrSize = v6 ? 16 : 4;
		addr.assign(buffer, buffer + addrSize);
		buffer += addrSize;

		uint16_t i = _byteswap_ushort(*reinterpret_cast<const uint16_t*>(buffer));
	}
};

void mtt::DhtCommunication::test()
{
	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	bool ipv6 = true;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock(io_service);
	sock.open(ipv6 ? udp::v6() : udp::v4());

	std::string myId(20,0);
	for (int i = 0; i < 20; i++)
	{
		myId[i] = 5 + i * 5;
	}

	std::string targetIdBase32 = "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetIdRaw = base32decode(targetIdBase32);

	PacketBuilder packet;
	packet.add("d1:ad2:id20:");
	packet.add(myId.data(), 20);
	packet.add("9:info_hash20:");
	packet.add(targetIdRaw.data(), 20);
	packet.add("e1:q9:get_peers1:t2:aa1:y1:qe");
	DataBuffer buffer = packet.getBuffer();

	std::vector<NodeInfo> receivedNodes;

	try
	{	
		auto message = sendUdpRequest(sock, resolver, buffer, dhtRoot, dhtRootPort, 5000, ipv6);

		if (!message.empty())
		{
			BencodeParser parser;
			parser.parse(message);

			if (parser.parsedData.isMap())
			{
				auto obj = parser.parsedData.dic->find("r");

				if (obj != parser.parsedData.dic->end() && obj->second.isMap())
				{
					auto nodesV4 = obj->second.dic->find("nodes");
					auto nodesV6 = obj->second.dic->find("nodes6");

					bool v6nodes = (nodesV6 != obj->second.dic->end());
					auto nodes = v6nodes ? nodesV6 : nodesV4;

					if (nodes != obj->second.dic->end() && nodes->second.type == mtt::BencodeParser::Object::Text)
					{
						auto& data = nodes->second.txt;

						for (size_t pos = 0; pos < data.size(); pos += 38)
						{
							NodeInfo info;
							info.parse(&data[pos], v6nodes);

							receivedNodes.push_back(info);
						}	
					}
				}
			}

		}
	}
	catch (const std::exception&e)
	{
		DHT_LOG("DHT exception: " << e.what() << "\n");
	}
}


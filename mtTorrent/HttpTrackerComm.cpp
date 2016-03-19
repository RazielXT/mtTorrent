#include "HttpTrackerComm.h"
#include <iomanip>
#include <iostream>
#include "BencodeParser.h"
#include "PacketHelper.h"

using namespace Torrent;

std::string url_encode(const std::string &s)
{
	static const char lookup[] = "0123456789ABCDEF";
	std::stringstream e;
	for (size_t i = 0, ix = s.length(); i < ix; i++)
	{
		const char& c = s[i];
		if ((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) //ABC...XYZ
			//(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}

Torrent::HttpTrackerComm::HttpTrackerComm()
{

}

void Torrent::HttpTrackerComm::setInfo(ClientInfo* c, TorrentInfo* t)
{
	client = c;
	torrent = t;
}

Torrent::AnnounceResponse Torrent::HttpTrackerComm::announceTracker(std::string host, std::string port)
{
	std::cout << "Announcing to HTTP tracker " << host << "\n";

	AnnounceResponse announceMsg;

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);

	tcp::socket sock(io_service);
	sock.open(tcp::v4());

	try
	{
		openTcpSocket(sock, resolver, host.data(), port.data());

		auto request = createAnnounceRequest(host, port);
		auto resp = sendTcpRequest(sock, request);

		announceMsg = getAnnounceResponse(resp);

		std::cout << "Http Tracker " << host << " returned peers:" << std::to_string(announceMsg.peers.size()) << ", p: " << std::to_string(announceMsg.seedCount) << ", l: " << std::to_string(announceMsg.leechCount) << "\n";
	}
	catch (const std::exception&e)
	{
		std::cout << "Https " << host << " exception: " << e.what() << "\n";
	}

	return announceMsg;
}

DataBuffer Torrent::HttpTrackerComm::createAnnounceRequest(std::string host, std::string port)
{
	std::string request = "GET /announce?info_hash=" + url_encode(std::string(torrent->infoHash.data(), 20)) + "&peer_id=" + url_encode(std::string(client->hashId, 20)) +
		"&port=" + std::to_string(client->listenPort) + "&uploaded=0&downloaded=0&left=" + std::to_string(torrent->fullSize) + 
		"&numwant=" + std::to_string(client->maxPeersPerRequest) + "&compact=1&no_peer_id=0&key=" + std::to_string(client->key) + "&event=started HTTP/1.0" + "\r\n" +
		"User-Agent: BitTorrent/3.4.2" + "\r\n" +
		"Connection: close" + "\r\n" +
		"Accept-Encoding: gzip, deflate" + "\r\n"
		"Host: " + host + ":" + port + "\r\n" +
		"Cache-Control: no-cache" + "\r\n" + "\r\n";

	return DataBuffer(request.begin(), request.end());
}

Torrent::AnnounceResponse Torrent::HttpTrackerComm::getAnnounceResponse(DataBuffer buffer)
{
	AnnounceResponse response;

	std::string success = "HTTP/1.1 200 OK";
	if (buffer.size() > success.length() && std::string(buffer.data(), success.length()) == success)
	{
		size_t dataStart = 0;

		for (size_t i = 0; i < buffer.size(); i++)
		{
			if (buffer.size() - i > 8 && buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
			{
				dataStart = i + 4;
				break;
			}
		}

		if (dataStart != 0)
		{
			BencodeParser parser;
			parser.parse(buffer.data() + dataStart, buffer.size() - dataStart - 4);

			if (parser.parsedData.type == BencodeParser::Object::Dictionary)
			{
				auto& data = parser.parsedData;

				auto interval  = data.getIntItem("min interval");
				if(!interval)
					interval = data.getIntItem("interval");

				response.interval = interval ? *interval : 5 * 60;

				auto seeds = data.getIntItem("complete");
				response.seedCount = seeds ? *seeds : 0;

				auto leechs = data.getIntItem("incomplete");
				response.leechCount = leechs ? *leechs : 0;

				auto peers = data.getTxtItem("peers");
				if (peers && peers->length() % 6 == 0)
				{
					PacketReader reader(peers->data(), peers->length());

					auto count = peers->length() / 6;
					for (size_t i = 0; i < count; i++)
					{		
						PeerInfo info;
						info.setIp(reader.pop32());
						info.port = reader.pop16();
						response.peers.push_back(info);
					}
				}
			}
		}
	}

	return response;
}


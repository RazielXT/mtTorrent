#include "HttpTrackerComm.h"
#include <iomanip>
#include <iostream>
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "Configuration.h"

using namespace mtt;

std::string url_encode(uint8_t* data, uint32_t size)
{
	static const char lookup[] = "0123456789ABCDEF";
	std::stringstream e;
	for (size_t i = 0, ix = size; i < ix; i++)
	{
		uint8_t& c = data[i];
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

mtt::HttpTrackerComm::HttpTrackerComm(TorrentFileInfo* t) : torrent(t)
{
}

mtt::AnnounceResponse mtt::HttpTrackerComm::announceTracker(std::string host, std::string port)
{
	TRACKER_LOG("Announcing to HTTP tracker " << host << "\n");

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

		TRACKER_LOG("Http Tracker " << host << " returned peers:" << std::to_string(announceMsg.peers.size()) << ", p: " << std::to_string(announceMsg.seedCount) << ", l: " << std::to_string(announceMsg.leechCount) << "\n");
	}
	catch (const std::exception&e)
	{
		TRACKER_LOG("Https " << host << " exception: " << e.what() << "\n");
	}

	return announceMsg;
}

DataBuffer mtt::HttpTrackerComm::createAnnounceRequest(std::string host, std::string port)
{
	std::string request = "GET /announce?info_hash=" + url_encode(torrent->info.hash, 20) + "&peer_id=" + url_encode(mtt::config::internal.hashId, 20) +
		"&port=" + std::to_string(mtt::config::external.listenPort) + "&uploaded=0&downloaded=0&left=" + std::to_string(torrent->info.fullSize) +
		"&numwant=" + std::to_string(mtt::config::external.maxPeersPerRequest) + "&compact=1&no_peer_id=0&key=" + std::to_string(mtt::config::internal.key) + "&event=started HTTP/1.0" + "\r\n" +
		"User-Agent: BitTorrent/3.4.2" + "\r\n" +
		"Connection: close" + "\r\n" +
		"Accept-Encoding: gzip, deflate" + "\r\n"
		"Host: " + host + ":" + port + "\r\n" +
		"Cache-Control: no-cache" + "\r\n" + "\r\n";

	return DataBuffer(request.begin(), request.end());
}

mtt::AnnounceResponse mtt::HttpTrackerComm::getAnnounceResponse(DataBuffer buffer)
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
						uint32_t addr = reader.pop32();
						response.peers.push_back(Addr((char*)&addr, reader.pop16(), false));
					}
				}
			}
		}
	}

	return response;
}


#include "HttpTrackerComm.h"
#include <iomanip>
#include <iostream>
#include "BencodeParser.h"
#include "PacketHelper.h"
#include "Configuration.h"


#define TRACKER_LOG(x) WRITE_LOG("HTTP tracker " << info.hostname << " " << x)

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

mtt::HttpTrackerComm::HttpTrackerComm()
{
}

mtt::HttpTrackerComm::~HttpTrackerComm()
{
	if (tcpComm)
		tcpComm->close();
}

void mtt::HttpTrackerComm::init(std::string host, std::string p, boost::asio::io_service& io, TorrentPtr t)
{
	info.hostname = host;
	port = p;
	torrent = t;

	tcpComm = std::make_shared<TcpAsyncStream>(io);
	tcpComm->onConnectCallback = std::bind(&HttpTrackerComm::onTcpConnected, this);
	tcpComm->onCloseCallback = std::bind(&HttpTrackerComm::onTcpClosed, this);
	tcpComm->onReceiveCallback = std::bind(&HttpTrackerComm::onTcpReceived, this);

	state = Initialized;

	tcpComm->init(host, port);
}

void mtt::HttpTrackerComm::fail()
{
	if (state == Announcing || state == Reannouncing)
	{
		if (state == Reannouncing)
			state = Alive;
		else
			state = Initialized;

		if (onFail)
			onFail();
	}
}

void mtt::HttpTrackerComm::onTcpClosed()
{
	fail();
}

void mtt::HttpTrackerComm::onTcpConnected()
{
	state = std::max(state, Alive);
}

void mtt::HttpTrackerComm::onTcpReceived()
{
	auto respData = tcpComm->getReceivedData();

	if (state == Announcing || state == Reannouncing)
	{
		mtt::AnnounceResponse announceResp;
		auto msgSize = readAnnounceResponse(respData, announceResp);

		if (msgSize == 0)
			return;
		else if (msgSize == -1)
		{
			fail();
			tcpComm->consumeData(respData.size());
			return;
		}
		else
		{
			tcpComm->consumeData(msgSize);
			state = Announced;

			info.leechers = announceResp.leechCount;
			info.seeds = announceResp.seedCount;
			info.peers = (uint32_t)announceResp.peers.size();
			info.announceInterval = announceResp.interval;

			TRACKER_LOG("received peers:" << announceResp.peers.size() << ", p: " << announceResp.seedCount << ", l: " << announceResp.leechCount);

			if (onAnnounceResult)
				onAnnounceResult(announceResp);
		}
	}
}

DataBuffer mtt::HttpTrackerComm::createAnnounceRequest(std::string host, std::string port)
{
	PacketBuilder builder(400);
	builder << "GET /announce?info_hash=" << url_encode(torrent->info.hash, 20);
	builder << "&peer_id=" << url_encode(mtt::config::internal.hashId, 20);
	builder << "&port=" << std::to_string(mtt::config::external.listenPort);
	builder << "&uploaded=0&downloaded=0&left=" << std::to_string(torrent->info.fullSize);
	builder << "&numwant=" << std::to_string(mtt::config::external.maxPeersPerRequest);
	builder << "&compact=1&no_peer_id=0&key=" << std::to_string(mtt::config::internal.key);
	builder << "&event=started HTTP/1.0\r\n";
	builder << "User-Agent: " << MT_NAME << "\r\n";
	builder << "Connection: close\r\n";
	builder << "Accept-Encoding: gzip, deflate\r\n";
	builder << "Host: " << host << ":" << port << "\r\n";
	builder << "Cache-Control: no-cache\r\n\r\n";

	return builder.getBuffer();
}

uint32_t mtt::HttpTrackerComm::readAnnounceResponse(DataBuffer& buffer, AnnounceResponse& response)
{
	auto info = readHttpHeader(buffer);

	if (!info.valid || !info.success)
		return -1;

	if (info.dataSize && info.dataStart && (info.dataStart + info.dataSize) <= buffer.size())
	{
		try
		{
			BencodeParser parser;
			parser.parse(buffer.data() + info.dataStart, info.dataSize);

			if (parser.parsedData.isMap())
			{
				auto& data = parser.parsedData;

				auto interval = data.getIntItem("min interval");
				if (!interval)
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
						response.peers.push_back(Addr(addr, reader.pop16()));
					}
				}
			}
		}
		catch (...)
		{
		}

		return info.dataStart + info.dataSize;
	}

	return 0;
}

mtt::HttpTrackerComm::HttpHeaderInfo mtt::HttpTrackerComm::readHttpHeader(DataBuffer& buffer)
{
	HttpHeaderInfo info;

	size_t pos = 0;
	while (pos + 1 < buffer.size())
	{
		if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
		{
			info.dataStart = (uint32_t)pos + 2;
			break;
		}

		std::string line;
		for (size_t i = pos + 1; i < buffer.size() - 1; i++)
		{
			if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			{
				line = std::string((char*)&buffer[pos], (char*)&buffer[i]);
				pos = i + 2;
				break;
			}
		}

		if (line.find_first_of(':') != std::string::npos)
		{
			auto vpos = line.find_first_of(':');
			info.headerParameters.push_back({ line.substr(0, vpos),line.substr(vpos+2) });
		}
		else
			info.headerParameters.push_back({line,""});
	}
	
	for (auto& p : info.headerParameters)
	{
		if (p.first == "Content-Length" && !p.second.empty())
			info.dataSize = std::stoul(p.second);
	}

	if (info.dataStart && !info.dataSize)
		info.valid = false;

	if (!info.headerParameters.empty() && info.headerParameters[0].first.find("200 OK") != std::string::npos)
		info.success = true;

	return info;
}

void mtt::HttpTrackerComm::announce()
{
	TRACKER_LOG("announcing");

	if (state == Announced)
		state = Reannouncing;
	else
		state = Announcing;

	auto request = createAnnounceRequest(info.hostname, port);

	tcpComm->write(request);
}

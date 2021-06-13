#include "UtpManager.h"
#include "Api/Configuration.h"
#include <set>

static mtt::utp::Manager* mgr = nullptr;

mtt::utp::Manager::Manager()
{
	mgr = this;
}

mtt::utp::Manager::~Manager()
{
	mgr = nullptr;
}

mtt::utp::Manager& mtt::utp::Manager::get()
{
	return *mgr;
}

void mtt::utp::Manager::start(uint16_t port)
{
	currentUdpPort = port;

	if (!active)
	{
		active = true;
		service.start(4);

		timeoutTimer = ScheduledTimer::create(service.io, [this]()
			{
				refresh();
				timeoutTimer->schedule(std::chrono::milliseconds(500));
			});

		timeoutTimer->schedule(std::chrono::milliseconds(500));
	}

}

void mtt::utp::Manager::stop()
{
	{
		std::lock_guard<std::mutex> guard(streamsMutex);
		streams.clear();
		active = false;
	}

	timeoutTimer->disable();
	service.stop();
}

mtt::utp::StreamPtr mtt::utp::Manager::createStream(const udp::endpoint& e, std::function<void(bool)> onResult)
{
	std::lock_guard<std::mutex> guard(streamsMutex);

	auto stream = std::make_shared<mtt::utp::Stream>(service.io);
	stream->onConnectCallback = onResult;
	stream->connect(e, currentUdpPort);

	streams.insert({ stream->getId(), stream });

	return stream;
}

bool mtt::utp::Manager::onUdpPacket(udp::endpoint& e, std::vector<DataBuffer*>& buffers)
{
	if (!active)
		return false;

	auto getUtpPacketHeaderSize = [](DataBuffer* data) -> uint32_t
	{
		if (data->size() < sizeof(utp::MessageHeader))
			return 0;

		auto header = reinterpret_cast<utp::MessageHeader*>(data->data());

		if (header->getVersion() != utp::MessageHeader::CurrentVersion || header->getType() > ST_SYN)
			return 0;

		return parseHeaderSize(header, data->size());
	};

	std::swap(usedBuffers, buffers);
	headerSizes.clear();
	buffers.clear();

	for (auto data : usedBuffers)
	{
		headerSizes.push_back(getUtpPacketHeaderSize(data));

		if(headerSizes.back() == 0)
			buffers.push_back(data);
	}

	if (buffers.size() == usedBuffers.size())
		return false;

	std::lock_guard<std::mutex> guard(streamsMutex);

	std::set<StreamPtr> affected;
	StreamPtr currentStream;	//cache last stream, usually batch of packets from same source
	uint16_t curentStreamId = 0;

	auto findStream = [this, &e, &currentStream, &curentStreamId, &affected](uint16_t id)
	{
		if (curentStreamId == id && currentStream)
			return true;

		auto stream = streams.find(id);
		while (stream != streams.end() && stream->first == id)
		{
			if (stream->second->getEndpoint() == e)
			{
				currentStream = stream->second;
				curentStreamId = id;
				affected.emplace(currentStream);
				return true;
			}
		}
		return false;
	};

	for (size_t i = 0; i < usedBuffers.size(); i++)
	{
		auto headerSize = headerSizes[i];
		if (headerSize == 0)
			continue;
		auto data = usedBuffers[i];
		auto header = reinterpret_cast<utp::MessageHeader*>(data->data());

		if (findStream(header->connection_id))
		{
			currentStream->readUdpPacket(*header, { data->data() + headerSize , data->size() - headerSize });
		}
		else if (header->getType() == ST_SYN && data->size() == headerSize)
		{
			onNewConnection(e, *header);
		}
	}

	for (auto s : affected)
	{
		s->readFinish();
	}

	return true;
}

void mtt::utp::Manager::refresh()
{
	std::lock_guard<std::mutex> guard(streamsMutex);

	TimePoint now = TimeClock::now();
	for (auto it = streams.begin(); it != streams.end();)
	{
		if (!it->second->refresh(now))
			it = streams.erase(it);
		else
			it++;
	}
}

void mtt::utp::Manager::onNewConnection(const udp::endpoint& e, const MessageHeader& header)
{
	auto stream = std::make_shared<utp::Stream>(service.io);
	streams.insert({ header.connection_id + 1, stream });

	if (onConnection)
		onConnection(stream);

	stream->connect(e, currentUdpPort, header);
}

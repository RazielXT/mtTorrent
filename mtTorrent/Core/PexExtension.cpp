#include "PexExtension.h"
#include "utils/SHA.h"
#include "utils/BencodeParser.h"
#include "utils/BencodeWriter.h"
#include "utils/PacketHelper.h"
#include "PeerCommunication.h"

#define BT_EXT_LOG(x) WRITE_GLOBAL_LOG(ExtensionProtocol, x)

using namespace mtt;
using namespace mtt::ext;

static void setBufferSizeHeader(DataBuffer& data)
{
	PacketBuilder::Assign32(data.data(), (uint32_t)(data.size() - sizeof(uint32_t)));
}

PeerExchange::LocalData::LocalData()
{
	dataTime = mtt::CurrentTimestamp();
}

static void createPexMessage(const PeerExchange::Message& source, DataBuffer& pexData, uint32_t(&droppedOffset)[2])
{
	droppedOffset[0] = 0;

	std::string data;
	data.reserve(source.addedPeers.size() * 6);

	pexData.resize(sizeof(uint32_t)); //length
	pexData.push_back(PeerMessage::Extended);
	pexData.push_back(0); //peer specific msg idx, to be filled

	BencodeWriter writer(std::move(pexData));
	writer.startMap();
	{
		if (!source.addedPeers.empty())
		{
			for (const auto& a : source.addedPeers)
				data += a.toData();
			writer.addRawItem("5:added", data);

			if (source.addedFlags.size == source.addedPeers.size())
				writer.addRawItemFromBuffer("7:added.f", (const char*)source.addedFlags.data, source.addedFlags.size);
		}

		if (!source.droppedPeers.empty())
		{
			droppedOffset[0] = (uint32_t)writer.data.size();

			data.clear();

			for (const auto& a : source.droppedPeers)
				data += a.toData();
			writer.addRawItem("7:dropped", data);

			droppedOffset[1] = (uint32_t)writer.data.size();
		}
	}
	writer.endMap();

	pexData = std::move(writer.data);
	setBufferSizeHeader(pexData);

	if (pexData.size() == 8)//just headers
		pexData.clear();
}

void PeerExchange::LocalData::update(const std::vector<PeerCommunication*>& peers)
{
	const auto now = mtt::CurrentTimestamp();
	if (now - UpdateFreq < dataTime)
		return;

	std::lock_guard<std::mutex> guard(dataMutex);

	dataTime = now;
	auto oldPeers = std::move(dataPeers);

	const size_t MaxPexPeers = 50;

	PeerExchange::Message msg;
	msg.addedPeers.reserve(std::min(MaxPexPeers, peers.size()));
	DataBuffer addedFlags;

	for (const auto p : peers)
	{
		auto addr = p->getStream()->getAddress();

		if (addedFlags.size() < MaxPexPeers)
		{
			if (!addr.ipv6)
			{
				msg.addedPeers.push_back(addr);
				uint8_t flags = 0;
				if (p->info.pieces.finished())
					flags |= PeerExchange::Seed;
				if (p->getStream()->isEncrypted())
					flags |= PeerExchange::SupportsEncryption;
				if (p->getStream()->isUtp())
					flags |= PeerExchange::SupportsUtp;
				if (p->ext.holepunch.enabled())
					flags |= PeerExchange::SupportsUth;

				addedFlags.push_back(flags);
			}
		}

		dataPeers.insert(addr);

		if (auto it = oldPeers.find(addr); it != oldPeers.end())
		{
			oldPeers.erase(it);
		}
	}

	msg.droppedPeers.reserve(oldPeers.size());
	for (auto& addr : oldPeers)
	{
		msg.droppedPeers.push_back(addr);
	}

	msg.addedFlags = addedFlags;

	createPexMessage(msg, pexData, droppedDataOffset);

	hash = BufferHash(pexData.data(), pexData.size());
}

bool PeerExchange::LocalData::empty() const
{
	std::lock_guard<std::mutex> guard(dataMutex);
	return pexData.empty();
}

DataBuffer PeerExchange::LocalData::getPexData() const
{
	std::lock_guard<std::mutex> guard(dataMutex);
	return pexData;
}

DataBuffer PeerExchange::LocalData::createFirstPexData() const
{
	std::lock_guard<std::mutex> guard(dataMutex);
	DataBuffer data = pexData;

	if (!data.empty() && data.size() > droppedDataOffset[1])
	{
		data.erase(data.begin() + droppedDataOffset[0], data.begin() + droppedDataOffset[1]);
		setBufferSizeHeader(data);
	}

	return data;
}

bool PeerExchange::Remote::load(const BufferView& data, Message& msg)
{
	BencodeParser parser;
	if (!parser.parse(data.data, data.size))
		return false;

	auto root = parser.getRoot();
	if (!root || !root->isMap())
		return false;

	if (auto added = root->popTxtItem("added"))
	{
		uint32_t pos = 0;
		msg.addedPeers.reserve(added->size / 6);

		std::lock_guard<std::mutex> guard(peersMutex);

		while (added->size - pos >= 6)
		{
			msg.addedPeers.emplace_back(Addr((const uint8_t*)added->data + pos, false));
			peers.insert(msg.addedPeers.back());
			pos += 6;
		}
	}

	if (auto addedF = root->popTxtItem("added.f"))
	{
		if (addedF->size == msg.addedPeers.size())
			msg.addedFlags = { (const uint8_t*)addedF->data, (std::size_t)addedF->size };
	}

	if (auto added6 = root->popTxtItem("added6"))
	{
		uint32_t pos = 0;
		msg.added6Peers.reserve(added6->size / 18);

		while (added6->size - pos >= 18)
		{
			msg.added6Peers.emplace_back(Addr((const uint8_t*)added6->data + pos, true));
			pos += 18;
		}
	}

	if (auto added6F = root->popTxtItem("added6.f"))
	{
		if (added6F->size == msg.added6Peers.size())
			msg.added6Flags = { (const uint8_t*)added6F->data, (std::size_t)added6F->size };
	}

	if (auto dropped = root->popTxtItem("dropped"))
	{
		uint32_t pos = 0;
		msg.droppedPeers.reserve(dropped->size / 6);

		while (dropped->size - pos >= 6)
		{
			msg.droppedPeers.emplace_back(Addr((const uint8_t*)dropped->data + pos, false));
			pos += 6;
		}
	}

	BT_EXT_LOG("PEX peers count " << msg.addedPeers.size());

	return true;
}

void PeerExchange::Remote::update(const PeerExchange::LocalData& source)
{
	if (lastHash == source.hash || source.hash == 0 || !peer)
		return;

	const auto now = mtt::CurrentTimestamp();
	if (lastSendTime && now - UpdateFreq < lastSendTime)
		return;

	DataBuffer pexData = lastSendTime ? source.getPexData() : source.createFirstPexData();
	pexData[5] = messageId; //peer specific msg idx
	peer->send(pexData);

	lastSendTime = now;
	lastHash = source.hash;
}

bool PeerExchange::Remote::wasConnected(const Addr& addr) const
{
	std::lock_guard<std::mutex> guard(peersMutex);
	return peers.find(addr) != peers.end();
}

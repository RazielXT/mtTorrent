#include "Dht/Communication.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include <fstream>

mtt::dht::Communication* comm;

#define DHT_LOG(x) WRITE_LOG(LogTypeDht, x)

mtt::dht::Communication::Communication() : responder(*this)
{
	udp = UdpAsyncComm::Get();
	udp->listen(std::bind(&Communication::onUnknownUdpPacket, this, std::placeholders::_1, std::placeholders::_2));
	comm = this;
	
	responder.table = table = std::make_shared<Table>();
}

mtt::dht::Communication::~Communication()
{
	udp.reset();
}

mtt::dht::Communication& mtt::dht::Communication::get()
{
	return *comm;
}

void mtt::dht::Communication::start()
{
	DHT_LOG("Start, my ID " << hexToString(mtt::config::getInternal().hashId, 20));

	service.start(2);

	load();

	refreshTable();

	loadDefaultRoots();

	findNode(mtt::config::getInternal().hashId);

	refreshTimer = ScheduledTimer::create(service.io, std::bind(&Communication::refreshTable, this));
	refreshTimer->schedule(5 * 60 + 5);

	//std::make_shared<Query::PingNodes>()->start(Addr({ 83,26,144,62 }, 44035) , &table, this);
}

void mtt::dht::Communication::stop()
{
	{
		std::lock_guard<std::mutex> guard(peersQueriesMutex);

		for (auto it = peersQueries.begin(); it != peersQueries.end(); it++)
			it->q->stop();

		peersQueries.clear();
	}

	if(refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	udp->removeListeners();
	service.stop();

	save();
}

void mtt::dht::Communication::removeListener(ResultsListener* listener)
{
	std::lock_guard<std::mutex> guard(peersQueriesMutex);

	for (auto it = peersQueries.begin(); it != peersQueries.end();)
	{
		if (it->listener == listener)
			it = peersQueries.erase(it);
		else
			it++;
	}
}

bool operator== (std::shared_ptr<mtt::dht::Query::DhtQuery> query, const uint8_t* hash)
{
	return memcmp(query->targetId.data, hash, 20) == 0;
}

void mtt::dht::Communication::findPeers(const uint8_t* hash, ResultsListener* listener)
{
	DHT_LOG("Start findPeers ID " << hexToString(hash, 20));

	{
		std::lock_guard<std::mutex> guard(peersQueriesMutex);

		for (auto it = peersQueries.begin(); it != peersQueries.end(); it++)
			if (it->q == hash)
				return;
	}

	QueryInfo info;
	info.q = std::make_shared<Query::GetPeers>();
	info.listener = listener;
	{
		std::lock_guard<std::mutex> guard(peersQueriesMutex);
		peersQueries.push_back(info);
	}

	info.q->start(hash, table, this);
}

void mtt::dht::Communication::stopFindingPeers(const uint8_t* hash)
{
	std::lock_guard<std::mutex> guard(peersQueriesMutex);

	for (auto it = peersQueries.begin(); it != peersQueries.end(); it++)
		if (it->q == hash)
		{
			peersQueries.erase(it);
			break;
		}
}

void mtt::dht::Communication::findNode(const uint8_t* hash)
{
	DHT_LOG("Start findNode ID " << hexToString(hash, 20));

	auto q = std::make_shared<Query::FindNode>();
	q->start(hash, table, this);
}

void mtt::dht::Communication::pingNode(Addr& addr)
{
	DHT_LOG("Start pingNode addr " << addr.toString());

	auto q = std::make_shared<Query::PingNodes>();
	q->start(addr, table, this);
}

uint32_t mtt::dht::Communication::onFoundPeers(const uint8_t* hash, std::vector<Addr>& values)
{
	std::lock_guard<std::mutex> guard(peersQueriesMutex);

	for (auto info : peersQueries)
	{
		if (info.listener && info.q == hash)
		{
			uint32_t c = info.listener->dhtFoundPeers(hash, values);
			return c;
		}
	}

	return (uint32_t)values.size();
}

void mtt::dht::Communication::findingPeersFinished(const uint8_t* hash, uint32_t count)
{
	DHT_LOG("FindingPeersFinished ID " << hexToString(hash, 20));

	std::lock_guard<std::mutex> guard(peersQueriesMutex);

	for (auto it = peersQueries.begin(); it != peersQueries.end(); it++)
		if (it->q == hash)
		{
			if(it->listener)
				it->listener->dhtFindingPeersFinished(hash, count);

			peersQueries.erase(it);
			break;
		}
}

UdpRequest mtt::dht::Communication::sendMessage(Addr& addr, DataBuffer& data, UdpResponseCallback response)
{
	return udp->sendMessage(data, addr, response);
}

void mtt::dht::Communication::stopMessage(UdpRequest r)
{
	udp->removeCallback(r);
}

void mtt::dht::Communication::sendMessage(udp::endpoint& endpoint, DataBuffer& data)
{
	udp->sendMessage(data, endpoint);
}

void mtt::dht::Communication::loadDefaultRoots()
{
	auto resolveFunc = [this]
	(const std::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver)
	{
		if (!error)
		{
			udp::resolver::iterator end;

			while (iterator != end)
			{
				pingNode(Addr{ iterator->endpoint().address(), iterator->endpoint().port() });

				iterator++;
			}
		}
	};

	for (auto& r : mtt::config::getInternal().dht.defaultRootHosts)
	{
		udp::resolver::query query(r.first, r.second);
		auto resolver = std::make_shared<udp::resolver>(service.io);
		resolver->async_resolve(query, std::bind(resolveFunc, std::placeholders::_1, std::placeholders::_2, resolver));
	}
}

void mtt::dht::Communication::refreshTable()
{
	DHT_LOG("Start refreshTable");

	responder.refresh();

	auto inactive = table->getInactiveNodes();

	if (!inactive.empty())
	{
		auto q = std::make_shared<Query::PingNodes>();
		q->start(inactive, table, this);
	}
}

void mtt::dht::Communication::save()
{
	auto saveFile = table->save();

	{
		std::ofstream out(mtt::config::getInternal().programFolderPath + "\\dht", std::ios_base::binary);
		out << saveFile;
	}
}

void mtt::dht::Communication::load()
{
	std::ifstream inFile(mtt::config::getInternal().programFolderPath + "\\dht", std::ios_base::binary | std::ios_base::in);

	if (inFile)
	{
		auto saveFile = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
		table->load(saveFile);
	}
}

void mtt::dht::Communication::announceTokenReceived(const uint8_t* hash, std::string& token, udp::endpoint& source)
{
	Query::AnnouncePeer(hash, token, source, this);
}

bool mtt::dht::Communication::onUnknownUdpPacket(udp::endpoint& e, std::vector<DataBuffer*>& data)
{
	for (auto d : data)
	{
		responder.handlePacket(e, *d);
	}
	return true;
}
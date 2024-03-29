#include "Dht/Communication.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include "utils/Filesystem.h"
#include <fstream>

mtt::dht::Communication* comm = nullptr;

#define DHT_LOG(x) WRITE_GLOBAL_LOG(Dht, x)
#define DHT_DETAIL_LOG(x) //WRITE_GLOBAL_LOG(Dht, x)

mtt::dht::Communication::Communication(UdpAsyncComm& udpComm) : responder(*this), udp(udpComm)
{
	comm = this;
	
	responder.table = table = std::make_shared<Table>();
}

mtt::dht::Communication::~Communication()
{
	if (refreshTimer)
		stop();
}

mtt::dht::Communication& mtt::dht::Communication::get()
{
	return *comm;
}

void mtt::dht::Communication::init()
{
	config::registerOnChangeCallback(config::ValueType::Dht, [this]()
		{
			if (mtt::config::getExternal().dht.enabled)
				start();
			else
				stop();
		});

	if (mtt::config::getExternal().dht.enabled)
		start();
}

void mtt::dht::Communication::start()
{
	DHT_DETAIL_LOG("Start, my ID " << hexToString(mtt::config::getInternal().hashId, 20));

	service.start(2);

	load();

	refreshTable();

	loadDefaultRoots();

	findNode(mtt::config::getInternal().hashId);

	refreshTimer = ScheduledTimer::create(service.io, [this]() { refreshTable(); return ScheduledTimer::Duration(0); });
	refreshTimer->schedule(ScheduledTimer::DurationSeconds(5 * 60 + 5));
}

void mtt::dht::Communication::stop()
{
	{
		std::lock_guard<std::mutex> guard(peersQueriesMutex);

		for (auto& peersQuery : peersQueries)
			peersQuery.q->stop();

		peersQueries.clear();
	}

	if (refreshTimer)
		refreshTimer->disable();
	refreshTimer = nullptr;

	udp.removeListeners();
	service.stop();

	save();
}

bool mtt::dht::Communication::active() const
{
	return refreshTimer != nullptr;
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
	DHT_DETAIL_LOG("Start findPeers ID " << hexToString(hash, 20));

	{
		std::lock_guard<std::mutex> guard(peersQueriesMutex);

		for (auto& peersQuery : peersQueries)
			if (peersQuery.q == hash)
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
	DHT_DETAIL_LOG("Start findNode ID " << hexToString(hash, 20));

	auto q = std::make_shared<Query::FindNode>();
	q->start(hash, table, this);
}

void mtt::dht::Communication::pingNode(const Addr& addr)
{
	DHT_DETAIL_LOG("Start pingNode addr " << addr);

	auto q = std::make_shared<Query::PingNodes>();
	q->start(addr, table, this);
}

uint32_t mtt::dht::Communication::onFoundPeers(const uint8_t* hash, const std::vector<Addr>& values)
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
	DHT_DETAIL_LOG("FindingPeersFinished ID " << hexToString(hash, 20));

	std::lock_guard<std::mutex> guard(peersQueriesMutex);

	for (auto it = peersQueries.begin(); it != peersQueries.end(); it++)
		if (it->q == hash)
		{
			if (it->listener)
				it->listener->dhtFindingPeersFinished(hash, count);

			peersQueries.erase(it);
			break;
		}
}

UdpRequest mtt::dht::Communication::sendMessage(const Addr& addr, const DataBuffer& data, UdpResponseCallback response)
{
	return udp.sendMessage(data, addr, response);
}

void mtt::dht::Communication::stopMessage(UdpRequest r)
{
	udp.removeCallback(r);
}

void mtt::dht::Communication::sendMessage(const udp::endpoint& endpoint, const DataBuffer& data)
{
	udp.sendMessage(data, endpoint);
}

void mtt::dht::Communication::loadDefaultRoots()
{
	auto resolveFunc = [this]
	(const std::error_code& error, udp::resolver::results_type results, std::shared_ptr<udp::resolver> resolver)
	{
		if (!error)
		{
			for (auto& r : results)
			{
				pingNode(Addr{ r.endpoint().address(), r.endpoint().port() });
			}
		}
	};

	for (auto& r : mtt::config::getInternal().dht.defaultRootHosts)
	{
		auto resolver = std::make_shared<udp::resolver>(service.io);
		resolver->async_resolve(r.first, r.second, std::bind(resolveFunc, std::placeholders::_1, std::placeholders::_2, resolver));
	}
}

void mtt::dht::Communication::refreshTable()
{
	DHT_DETAIL_LOG("Start refreshTable");

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
		std::ofstream out(mtt::config::getInternal().programFolderPath + pathSeparator + "dht", std::ios_base::binary);
		out << saveFile;
	}
}

void mtt::dht::Communication::load()
{
	std::ifstream inFile(mtt::config::getInternal().programFolderPath + pathSeparator + "dht", std::ios_base::binary | std::ios_base::in);

	if (inFile)
	{
		auto saveFile = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
		table->load(saveFile);
	}
}

void mtt::dht::Communication::announceTokenReceived(const uint8_t* hash, const std::string& token, const udp::endpoint& source)
{
	Query::AnnouncePeer(hash, token, source, this);
}

bool mtt::dht::Communication::onUdpPacket(udp::endpoint& e, std::vector<BufferView>& data)
{
	for (auto d : data)
	{
		responder.handlePacket(e, d);
	}
	return true;
}

uint32_t mtt::dht::Communication::getNodesCount() const
{
	return table->getActiveNodesCount();
}

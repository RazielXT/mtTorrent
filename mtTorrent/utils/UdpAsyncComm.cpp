#include "UdpAsyncComm.h"
#include "Logging.h"
#include "Configuration.h"

#define UDP_LOG(x) WRITE_LOG(LogTypeUdpMgr, x)

std::shared_ptr<UdpAsyncComm> UdpAsyncComm::Get()
{
	static std::shared_ptr<UdpAsyncComm> ptr;

	if (!ptr)
	{
		ptr = std::make_shared<UdpAsyncComm>();
		ptr->setBindPort(mtt::config::getExternal().connection.udpPort);
		ptr->pool.start(4);
	}

	return ptr;
}

void UdpAsyncComm::setBindPort(uint16_t port)
{
	bindPort = port;
}

void UdpAsyncComm::listen(UdpPacketCallback receive)
{
	if (!listener)
		startListening();

	onUnhandledReceive = receive;
}

UdpRequest UdpAsyncComm::create(std::string& host, std::string& port)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);
	c->setAddress(host, port);
	c->setBindPort(bindPort);

	return c;
}

UdpRequest UdpAsyncComm::sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpResponseCallback response, bool ipv6)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);

	if (response)
		addPendingResponse(data, c, response);

	c->setAddress(host, port, ipv6);
	c->setBindPort(bindPort);
	c->write(data);

	return c;
}

void UdpAsyncComm::sendMessage(DataBuffer& data, UdpRequest c, UdpResponseCallback response, uint32_t timeout)
{
	if (response)
		addPendingResponse(data, c, response, timeout);

	c->write(data);
}

UdpRequest UdpAsyncComm::sendMessage(DataBuffer& data, Addr& addr, UdpResponseCallback response)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);

	if(response)
		addPendingResponse(data, c, response);

	c->setAddress(addr);
	c->setBindPort(bindPort);
	c->write(data);

	return c;
}

void UdpAsyncComm::sendMessage(DataBuffer& data, udp::endpoint& endpoint)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);
	c->setAddress(endpoint);
	c->setBindPort(bindPort);
	c->write(data);
}

void UdpAsyncComm::removeCallback(UdpRequest target)
{
	std::lock_guard<std::mutex> guard(responsesMutex);

	for(auto it = pendingResponses.begin(); it != pendingResponses.end(); it++)
	{
		if ((*it)->client == target)
		{
			pendingResponses.erase(it);
			break;
		}
	}
}

void UdpAsyncComm::addPendingResponse(DataBuffer& data, UdpRequest c, UdpResponseCallback response, uint32_t timeout)
{
	if (!listener)
		startListening();

	auto info = std::make_shared<ResponseRetryInfo>();
	info->client = c;
	info->defaultTimeout = timeout;
	info->timeoutTimer = std::make_shared<boost::asio::deadline_timer>(pool.io);
	info->timeoutTimer->expires_from_now(boost::posix_time::seconds(timeout));
	info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, info));
	info->onResponse = response;

	c->onCloseCallback = std::bind(&UdpAsyncComm::onUdpClose, this, std::placeholders::_1);

	std::lock_guard<std::mutex> guard(responsesMutex);

	pendingResponses.push_back(info);
}

UdpRequest UdpAsyncComm::findPendingConnection(UdpRequest source)
{
	UdpRequest c;

	std::lock_guard<std::mutex> guard(responsesMutex);

	auto it = pendingResponses.begin();
	for (auto& r : pendingResponses)
		if ((*it)->client == source)
			return (*it)->client;

	return c;
}

void UdpAsyncComm::onUdpReceive(udp::endpoint& source, DataBuffer& data)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if ((*it)->client->getEndpoint() == source)
			{
				foundPendingResponses.push_back(*it);
				it = pendingResponses.erase(it);
			}
			else
				++it;
		}
	}

	bool handled = false;

	for (auto r : foundPendingResponses)
	{
		if (!handled && r->onResponse(r->client, &data))
		{
			UDP_LOG(r->client->getName() << " successfully handled, removing");

			handled = true;
			r->reset();
		}
		else
		{
			UDP_LOG(r->client->getName() << " unsuccessfully handled");

			std::lock_guard<std::mutex> guard(responsesMutex);
			pendingResponses.push_back(r);
		}
	}

	if (!handled && onUnhandledReceive)
		onUnhandledReceive(source, data);
}

void UdpAsyncComm::onUdpClose(UdpRequest source)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if ((*it)->client == source)
			{
				foundPendingResponses.push_back(*it);
				it = pendingResponses.erase(it);
			}
			else
				++it;
		}
	}

	for (auto r : foundPendingResponses)
	{
		r->reset();
		r->onResponse(source, nullptr);
	}

	UDP_LOG(source->getName() << " closed, is handled response:" << !foundPendingResponses.empty());
}

void UdpAsyncComm::startListening()
{
	listener = std::make_shared<UdpAsyncReceiver>(pool.io, bindPort, false);
	listener->receiveCallback = std::bind(&UdpAsyncComm::onUdpReceive, this, std::placeholders::_1, std::placeholders::_2);
	listener->listen();
}

void UdpAsyncComm::checkTimeout(std::shared_ptr<ResponseRetryInfo> info)
{
	UDP_LOG("checking timer for " << info->client->getName());

	if (info->retries == 255)
		return;

	if (info->timeoutTimer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		if (info->retries > 0)
		{
			UDP_LOG(info->client->getName() << " response timeout");
			info->timeoutTimer->expires_at(boost::posix_time::pos_infin);
			onUdpClose(info->client);
		}
		else
		{
			UDP_LOG(info->client->getName() << " request retry");
			info->retries++;
			info->timeoutTimer->expires_from_now(boost::posix_time::seconds(info->retries + info->defaultTimeout));
			info->client->write();
		}
	}

	info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, info));
}

void UdpAsyncComm::ResponseRetryInfo::reset()
{
	retries = 255;
	timeoutTimer->cancel();
}

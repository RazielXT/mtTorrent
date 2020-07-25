#include "UdpAsyncComm.h"
#include "Logging.h"
#include "Configuration.h"

#define UDP_LOG(x) WRITE_LOG(LogTypeUdpMgr, x)

std::shared_ptr<UdpAsyncComm> ptr;

std::shared_ptr<UdpAsyncComm> UdpAsyncComm::Get()
{
	if (!ptr)
	{
		ptr = std::make_shared<UdpAsyncComm>();
		ptr->setBindPort(mtt::config::getExternal().connection.udpPort);
		ptr->pool.start(4);
	}

	return ptr;
}

void UdpAsyncComm::Deinit()
{
	if (ptr)
	{
		ptr->removeListeners();
		ptr->listener->stop();
		ptr->listener.reset();
		ptr->pool.stop();
		ptr.reset();
	}
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

void UdpAsyncComm::removeListeners()
{
	std::lock_guard<std::mutex> guard(respondingMutex);

	{
		std::lock_guard<std::mutex> guard(responsesMutex);
		pendingResponses.clear();
	}

	onUnhandledReceive = nullptr;
}

UdpRequest UdpAsyncComm::create(const std::string& host, const std::string& port)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);
	c->setAddress(host, port);
	c->setBindPort(bindPort);

	return c;
}

UdpRequest UdpAsyncComm::sendMessage(DataBuffer& data, const std::string& host, const std::string& port, UdpResponseCallback response, bool ipv6, uint32_t timeout, bool anySource)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);

	if (response)
		addPendingResponse(data, c, response, timeout, anySource);

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
	std::lock_guard<std::mutex> guard(respondingMutex);
	std::lock_guard<std::mutex> guard2(responsesMutex);

	for(auto it = pendingResponses.begin(); it != pendingResponses.end(); it++)
	{
		if ((*it)->client == target)
		{
			pendingResponses.erase(it);
			break;
		}
	}
}

void UdpAsyncComm::addPendingResponse(DataBuffer& data, UdpRequest c, UdpResponseCallback response, uint32_t timeout, bool anySource)
{
	if (!listener)
		startListening();

	auto info = std::make_shared<ResponseRetryInfo>();
	info->client = c;
	info->defaultTimeout = timeout;
	info->timeoutTimer = std::make_shared<asio::steady_timer>(pool.io);
	info->timeoutTimer->expires_from_now(std::chrono::seconds(timeout));
	info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, c, std::placeholders::_1));
	info->anySource = anySource;
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

void UdpAsyncComm::onUdpReceiveBuffers(udp::endpoint& source, std::vector<DataBuffer*>& buffers)
{
	std::vector<DataBuffer*> unhandled;

	for (auto data : buffers)
	{
		if (!onUdpReceive(source, *data))
			unhandled.push_back(data);
	}

	if (buffers.size() && onUnhandledReceive)
		onUnhandledReceive(source, unhandled);
}

bool UdpAsyncComm::onUdpReceive(udp::endpoint& source, DataBuffer& data)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	std::lock_guard<std::mutex> guard(respondingMutex);

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if ((*it)->client->getEndpoint() == source || (*it)->anySource)
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

	return handled;
}

void UdpAsyncComm::onUdpClose(UdpRequest source)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	std::lock_guard<std::mutex> guard(respondingMutex);

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
	listener->receiveCallback = std::bind(&UdpAsyncComm::onUdpReceiveBuffers, this, std::placeholders::_1, std::placeholders::_2);
	listener->listen();
}

void UdpAsyncComm::checkTimeout(UdpRequest client, const asio::error_code& error)
{
	if (error)
		return;

	std::shared_ptr<ResponseRetryInfo> info;
	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		for (auto r : pendingResponses)
		{
			if (r->client == client)
			{
				info = r;
				break;
			}
		}
	}

	if (!info || info->retries == 255)
		return;

	UDP_LOG("checking timer for " << info->client->getName());

	if (info->retries > 0)
	{
		UDP_LOG(info->client->getName() << " response timeout");
		onUdpClose(info->client);
	}
	else
	{
		UDP_LOG(info->client->getName() << " request retry");
		info->retries++;
		info->client->write();
		info->timeoutTimer->expires_from_now(std::chrono::seconds(info->retries + info->defaultTimeout));
		info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, client, std::placeholders::_1));
	}
}

void UdpAsyncComm::ResponseRetryInfo::reset()
{
	client.reset();
	retries = 255;
	timeoutTimer->cancel();
}

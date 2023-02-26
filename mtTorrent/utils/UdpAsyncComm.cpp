#include "UdpAsyncComm.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_GLOBAL_LOG(UdpMgr, x)

UdpAsyncComm* ptr = nullptr;

UdpAsyncComm::UdpAsyncComm()
{
	ptr = this;
}

UdpAsyncComm::~UdpAsyncComm()
{
	deinit();
}

UdpAsyncComm& UdpAsyncComm::Get()
{
	return *ptr;
}

void UdpAsyncComm::deinit()
{
	if (ptr)
	{
		ptr->removeListeners();

		if (ptr->listener)
		{
			ptr->listener->stop();
			ptr->listener.reset();
		}

		ptr->pool.stop();
		ptr = nullptr;
	}
}

void UdpAsyncComm::setBindPort(uint16_t port)
{
	if (bindPort != port)
	{
		bindPort = port;

		if (listener)
			startListening();
	}
}

void UdpAsyncComm::listen(UdpPacketCallback receive)
{
	pool.start(4);

	{
		std::lock_guard<std::mutex> guard(respondingMutex);
		onUnhandledReceive = receive;
	}

	startListening();
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

UdpRequest UdpAsyncComm::sendMessage(const DataBuffer& data, const Addr& addr, UdpResponseCallback response, uint32_t timeout)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(pool.io);

	if (response)
		addPendingResponse(c, response, timeout);

	c->setAddress(addr);
	c->setBindPort(bindPort);
	c->write(data);

	return c;
}

void UdpAsyncComm::sendMessage(const DataBuffer& data, UdpRequest c, UdpResponseCallback response, uint32_t timeout)
{
	if (response)
		addPendingResponse(c, response, timeout);

	c->write(data);
}

void UdpAsyncComm::sendMessage(const DataBuffer& data, const udp::endpoint& endpoint)
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
			target->close();
			pendingResponses.erase(it);
			break;
		}
	}
}

void UdpAsyncComm::addPendingResponse(UdpRequest c, UdpResponseCallback response, uint32_t timeout)
{
	auto info = std::make_shared<ResponseRetryInfo>();
	info->client = c;
	info->defaultTimeout = timeout;
	info->timeoutTimer = std::make_shared<asio::steady_timer>(pool.io);
	info->timeoutTimer->expires_after(std::chrono::seconds(timeout));
	info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, c, std::placeholders::_1));
	info->onResponse = response;

	c->onResponse = std::bind(&UdpAsyncComm::onDirectUdpReceive, this, std::placeholders::_1, std::placeholders::_2);
	c->onCloseCallback = [this](UdpRequest c) { std::lock_guard<std::mutex> guard(respondingMutex); onUdpClose(c); };

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

void UdpAsyncComm::onUdpReceiveBuffers(udp::endpoint& source, const std::vector<BufferView>& buffers)
{
	std::vector<BufferView> unhandled;

	for (auto data : buffers)
	{
		if (!onUdpReceive(source, data))
			unhandled.push_back(data);
	}

	if (!buffers.empty() && onUnhandledReceive)
		onUnhandledReceive(source, unhandled);
}

void UdpAsyncComm::onDirectUdpReceive(UdpRequest client, const BufferView& data)
{
	if (!data.data)
		return;

	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	std::lock_guard<std::mutex> guard(respondingMutex);

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if ((*it)->client == client)
			{
				foundPendingResponses.emplace_back(std::move(*it));
				it = pendingResponses.erase(it);
			}
			else
				++it;
		}
	}

	bool handled = false;

	for (const auto& r : foundPendingResponses)
	{
		if (!handled && r->onResponse(r->client, data))
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
}

bool UdpAsyncComm::onUdpReceive(udp::endpoint& source, const BufferView& data)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	std::lock_guard<std::mutex> guard(respondingMutex);

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

	for (const auto& r : foundPendingResponses)
	{
		if (!handled && r->onResponse(r->client, data))
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

	if (!handled)
		handled = false;
	return handled;
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

	for (const auto& r : foundPendingResponses)
	{
		r->reset();
		r->onResponse(source, {});
	}

	UDP_LOG(source->getName() << " closed, is handled response:" << !foundPendingResponses.empty());
}

void UdpAsyncComm::startListening()
{
	std::lock_guard<std::mutex> guard(respondingMutex);

	if (listener)
		listener->stop();

	if (!bindPort)
		return;

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

		for (const auto& r : pendingResponses)
		{
			if (r->client == client)
			{
				info = r;
				break;
			}
		}
	}

	std::lock_guard<std::mutex> guard(respondingMutex);

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
		info->timeoutTimer->expires_after(std::chrono::seconds(info->retries + info->defaultTimeout));
		info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, client, std::placeholders::_1));
	}
}

UdpAsyncComm::ResponseRetryInfo::~ResponseRetryInfo()
{
	if (client)
		client->close();
}

void UdpAsyncComm::ResponseRetryInfo::reset()
{
	retries = 255;
	timeoutTimer->cancel();
	timeoutTimer = nullptr;
}

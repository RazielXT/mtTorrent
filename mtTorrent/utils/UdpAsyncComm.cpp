#include "UdpAsyncComm.h"
#include "Logging.h"
#include "Configuration.h"

#define UDP_LOG(x) WRITE_LOG("UDP MGR: " << x)

UdpAsyncComm::UdpAsyncComm(boost::asio::io_service& io_service, uint16_t port) : io(io_service), bindPort(port)
{
}

void UdpAsyncComm::listen(UdpResponseCallback receive)
{
	if (!listener)
		startListening();

	onUnhandledReceive = receive;
}

UdpRequest UdpAsyncComm::create(std::string& host, std::string& port)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(io);
	c->setAddress(host, port);
	c->setBindPort(bindPort);

	return c;
}

UdpRequest UdpAsyncComm::sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpResponseCallback response)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(io);

	if (response)
		addPendingResponse(data, c, response);

	c->setAddress(host, port);
	c->setBindPort(bindPort);
	c->write(data);

	return c;
}

void UdpAsyncComm::sendMessage(DataBuffer& data, UdpRequest c, UdpResponseCallback response)
{
	if (response)
		addPendingResponse(data, c, response);

	c->write(data);
}

UdpRequest UdpAsyncComm::sendMessage(DataBuffer& data, Addr& addr, UdpResponseCallback response)
{
	UdpRequest c = std::make_shared<UdpAsyncWriter>(io);

	if(response)
		addPendingResponse(data, c, response);

	c->setAddress(addr);
	c->setBindPort(bindPort);
	c->write(data);

	return c;
}

void UdpAsyncComm::addPendingResponse(DataBuffer& data, UdpRequest c, UdpResponseCallback response)
{
	if (!listener)
		startListening();

	auto info = std::make_shared<ResponseRetryInfo>();
	info->client = c;
	info->timeoutTimer = std::make_shared<boost::asio::deadline_timer>(io);
	info->timeoutTimer->async_wait(std::bind(&UdpAsyncComm::checkTimeout, this, info));
	info->timeoutTimer->expires_from_now(boost::posix_time::seconds(1));
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

void UdpAsyncComm::onUdpReceive(UdpRequest source, DataBuffer& data)
{
	std::vector<std::shared_ptr<ResponseRetryInfo>> foundPendingResponses;

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if ((*it)->client->getEndpoint() == source->getEndpoint())
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
			UDP_LOG(source->getName() << " successfully handled, removing");

			handled = true;
			r->reset();
		}
		else
		{
			UDP_LOG(source->getName() << " unsuccessfully handled");

			std::lock_guard<std::mutex> guard(responsesMutex);
			pendingResponses.push_back(r);
		}
	}

	if (!handled && onUnhandledReceive)
		onUnhandledReceive(source, &data);
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

	UDP_LOG(source->getName() << " closed, handled response:" << !foundPendingResponses.empty());

	if (foundPendingResponses.empty() && onUnhandledReceive)
		onUnhandledReceive(source, nullptr);
}

void UdpAsyncComm::startListening()
{
	listener = std::make_shared<UdpAsyncReceiver>(io, bindPort, false);
	listener->receiveCallback = std::bind(&UdpAsyncComm::onUdpReceive, this, std::placeholders::_1, std::placeholders::_2);
	listener->listen();
}

void UdpAsyncComm::checkTimeout(std::shared_ptr<ResponseRetryInfo> info)
{
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
			info->timeoutTimer->expires_from_now(boost::posix_time::seconds(1));
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

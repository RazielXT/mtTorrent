#include "UdpAsyncMgr.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG("UDP MGR: " << x)

UdpAsyncMgr::UdpAsyncMgr(boost::asio::io_service& io_service) : io(io_service)
{

}

void UdpAsyncMgr::setImplicitPort(uint16_t port)
{
	implicitPort = port;
}

void UdpAsyncMgr::listen(uint16_t port, UdpConnectionCallback received)
{

}

UdpConnection UdpAsyncMgr::create(std::string& host, std::string& port)
{
	UdpConnection c = std::make_shared<UdpAsyncClient>(io);

	if (implicitPort)
		c->setImplicitPort(implicitPort);

	c->setAddress(host, port);

	return c;
}

UdpConnection UdpAsyncMgr::sendMessage(DataBuffer& data, std::string& host, std::string& port, UdpConnectionCallback response)
{
	UdpConnection c = std::make_shared<UdpAsyncClient>(io);

	if (implicitPort)
		c->setImplicitPort(implicitPort);

	if (response)
		addPendingResponse(data, c, response);

	c->setAddress(host, port);
	c->write(data);

	return c;
}

void UdpAsyncMgr::sendMessage(DataBuffer& data, UdpConnection c, UdpConnectionCallback response)
{
	if (response)
		addPendingResponse(data, c, response);

	c->write(data);
}

UdpConnection UdpAsyncMgr::sendMessage(DataBuffer& data, Addr& addr, UdpConnectionCallback response)
{
	UdpConnection c = std::make_shared<UdpAsyncClient>(io);

	if (implicitPort)
		c->setImplicitPort(implicitPort);

	if(response)
		addPendingResponse(data, c, response);

	c->setAddress(addr);
	c->write(data);

	return c;
}

void UdpAsyncMgr::addPendingResponse(DataBuffer& data, UdpConnection c, UdpConnectionCallback response)
{
	auto info = std::make_shared<ResponseRetryInfo>();
	info->client = c;
	info->writeData = data;
	info->timeoutTimer = std::make_shared<boost::asio::deadline_timer>(io);
	info->timeoutTimer->async_wait(std::bind(&UdpAsyncMgr::checkTimeout, this, info));
	info->timeoutTimer->expires_from_now(boost::posix_time::seconds(3));
	info->onResponse = response;

	c->onReceiveCallback = std::bind(&UdpAsyncMgr::onUdpReceive, this, std::placeholders::_1, std::placeholders::_2);
	c->onCloseCallback = std::bind(&UdpAsyncMgr::onUdpClose, this, std::placeholders::_1);

	std::lock_guard<std::mutex> guard(responsesMutex);

	pendingResponses.push_back(info);
}

UdpConnection UdpAsyncMgr::findPendingConnection(UdpConnection source)
{
	UdpConnection c;

	std::lock_guard<std::mutex> guard(responsesMutex);

	auto it = pendingResponses.begin();
	for (auto& r : pendingResponses)
		if ((*it)->client == source)
			return (*it)->client;

	return c;
}

void UdpAsyncMgr::onUdpReceive(UdpConnection source, DataBuffer& data)
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

	bool handled = false;

	for (auto r : foundPendingResponses)
	{
		if (!handled && r->onResponse(source, &data))
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

	if (!handled && onReceive)
		onReceive(source, &data);
}

void UdpAsyncMgr::onUdpClose(UdpConnection source)
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

	if (foundPendingResponses.empty() && onReceive)
		onReceive(source, nullptr);
}

void UdpAsyncMgr::checkTimeout(std::shared_ptr<ResponseRetryInfo> info)
{
	if (info->writeRetries == 255)
		return;

	if (info->timeoutTimer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		if (info->writeRetries >= 1)
		{
			UDP_LOG(info->client->getName() << " request timeout");
			info->timeoutTimer->expires_at(boost::posix_time::pos_infin);
			onUdpClose(info->client);
		}
		else
		{
			info->writeRetries++;
			UDP_LOG(info->client->getName() << " retry");
			info->client->write(info->writeData);
		}
	}

	info->timeoutTimer->async_wait(std::bind(&UdpAsyncMgr::checkTimeout, this, info));
}

void UdpAsyncMgr::ResponseRetryInfo::reset()
{
	writeRetries = 255;
	timeoutTimer->cancel();
}

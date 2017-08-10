#include "UdpAsyncMgr.h"

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
	ResponseRetryInfo info;
	info.client = c;
	info.writeData = data;
	info.timeoutTimer = std::make_shared<boost::asio::deadline_timer>(io);
	info.timeoutTimer->async_wait(std::bind(&ResponseRetryInfo::checkTimeout, &info));
	info.timeoutTimer->expires_from_now(boost::posix_time::seconds(3));

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
		if (it->client == source)
			return it->client;

	return c;
}

void UdpAsyncMgr::onUdpReceive(UdpConnection source, DataBuffer& data)
{
	bool handled = false;

	{
		bool left = false;

		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if (it->client == source)
			{
				if (!handled && it->onResponse(it->client, &data))
				{
					it = pendingResponses.erase(it);
					handled = true;
				}
				else
					left = true;
			}
			else
				++it;
		}

		if (!left)
		{
			source->onReceiveCallback = nullptr;
			source->onCloseCallback = nullptr;
		}
	}

	if (!handled && onReceive)
		onReceive(source, &data);
}

void UdpAsyncMgr::onUdpClose(UdpConnection source)
{
	bool handled = false;

	{
		std::lock_guard<std::mutex> guard(responsesMutex);

		auto it = pendingResponses.begin();
		while (it != pendingResponses.end())
		{
			if (it->client == source)
			{
				it->onResponse(it->client, nullptr);
				it = pendingResponses.erase(it);
				handled = true;
			}
			else
				++it;
		}
	}

	if (!handled && onReceive)
		onReceive(source, nullptr);
}

void UdpAsyncMgr::ResponseRetryInfo::checkTimeout()
{
	if (timeoutTimer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		if (writeRetries >= 1)
		{
			timeoutTimer->expires_at(boost::posix_time::pos_infin);
		}
		else
		{
			writeRetries++;
			client->write(writeData);
		}
	}

	timeoutTimer->async_wait(std::bind(&ResponseRetryInfo::checkTimeout, this));
}

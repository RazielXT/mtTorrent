#include "TcpAsyncStream.h"

#define TCP_LOG(x) WRITE_LOG(x)
#define TCP_LOG_DETAILED(x) //WRITE_LOG(x)

TcpAsyncStream::TcpAsyncStream(asio::io_context& io) : io_context(io), socket(io)
{
	timeoutTimer = std::make_unique<asio::steady_timer>(io);
	setBandwidthChannels(nullptr, 0);
	CREATE_LOG(TcpStream);
	readBuffer.log = log.get();
}

TcpAsyncStream::~TcpAsyncStream()
{
}

void TcpAsyncStream::connect(const uint8_t* ip, uint16_t port, bool ipv6)
{
	if (state != Clear)
		return; startReceive();

	info.address.set(ip, port, ipv6); 
	info.addressResolved = true;
	info.host = info.address.toString();

	timeoutTimer->expires_after(std::chrono::seconds(10));
	timeoutTimer->async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	connectByAddress();
}

void TcpAsyncStream::connect(const std::string& ip, uint16_t port)
{
	if (state != Clear)
		return;

	asio::error_code ec;
	info.address.set(asio::ip::make_address(ip, ec), port);
	info.addressResolved = true;
	info.host = ip;

	connectByAddress();
}

void TcpAsyncStream::init(const std::string& hostname, const std::string& port)
{
	info.host = hostname;
	info.address.port = (uint16_t)strtoul(port.data(), nullptr, 10);
}

void TcpAsyncStream::close(bool immediate)
{
	if (immediate)
	{
		onConnectCallback = nullptr;
		onReceiveCallback = nullptr;
		onCloseCallback = nullptr;
	}

	if (state == Disconnected)
		return;

	asio::post(io_context, std::bind(&TcpAsyncStream::do_close, shared_from_this()));
}

void TcpAsyncStream::write(const DataBuffer& data)
{
	asio::post(io_context, std::bind(&TcpAsyncStream::do_write, shared_from_this(), data));
}

void TcpAsyncStream::write(DataBuffer&& data)
{
	asio::post(io_context, std::bind(&TcpAsyncStream::do_write, shared_from_this(), std::move(data)));
}

const std::string& TcpAsyncStream::getHostname() const
{
	return info.host;
}

const Addr& TcpAsyncStream::getAddress() const
{
	return info.address;
}

uint64_t TcpAsyncStream::getReceivedDataCount() const
{
	return readBuffer.receivedCounter;
}

void TcpAsyncStream::setBandwidthPriority(int p)
{
	priority = p;
}

void TcpAsyncStream::setMinBandwidthRequest(uint32_t size)
{
	expecting_size = size;
}

std::string TcpAsyncStream::name()
{
	return getHostname();
}

void TcpAsyncStream::setBandwidthChannels(BandwidthChannel** bwc, uint32_t count)
{
	bwChannelsCount = count;
	for (uint32_t i = 0; i < 2; i++)
	{
		if (i < count)
			bwChannels[i] = bwc[i];
		else
			bwChannels[i] = nullptr;
	}
}

void TcpAsyncStream::connectByHostname()
{
	TCP_LOG("resolving hostname");

	state = Connecting;

	auto resolver = std::make_shared<tcp::resolver>(io_context);
	resolver->async_resolve(info.host, std::to_string(info.address.port), std::bind(&TcpAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void TcpAsyncStream::connectByAddress()
{
	TCP_LOG("connecting");

	state = Connecting;

	socket.async_connect(info.address.toTcpEndpoint(), std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncStream::initializeInfo()
{
	TCP_LOG("initializeInfo");

	std::error_code ec;
	auto endpoint = socket.remote_endpoint(ec);
	if (ec)
	{
		postFail("remote_endpoint", ec);
		return;
	}

	state = Connected;

	info.address.set(endpoint.address(), endpoint.port());
	info.addressResolved = true;
	info.host = endpoint.address().to_string();

	NAME_LOG(getHostname());
}

void TcpAsyncStream::setAsConnected()
{
	TCP_LOG("connected");

	std::error_code ec;
	socket.non_blocking(true, ec);
	if (ec)
	{
		postFail("non_blocking", ec);
		return;
	}

	{
		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		if (!write_msgs.empty())
		{
			TCP_LOG("writing " << write_msgs.front().size() << " bytes");

			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}

	startReceive();

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onConnectCallback)
			onConnectCallback();
	}
}

void TcpAsyncStream::postFail(const char* place, const std::error_code& error)
{
	{
		std::lock_guard<std::mutex> guard(receive_mutex);

		if (state == Disconnected)
			return;

		state = Disconnected;
		timeoutTimer = nullptr;
	}

	try
	{
		if (socket.is_open())
		{
			asio::error_code ec;
			socket.close(ec);
		}
	}
	catch (...)
	{
	}

	if (error)
		TCP_LOG("error on " << place << ": " << error.message())
	else
		TCP_LOG("end on " << place);

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onCloseCallback)
			onCloseCallback(error.value());

		onConnectCallback = nullptr;
		onCloseCallback = nullptr;
		onReceiveCallback = nullptr;
	}
}

void TcpAsyncStream::handle_connect(const std::error_code& error)
{
	if (!error)
	{
		initializeInfo();
		setAsConnected();
	}
	else
	{
		postFail("Connect", error);
	}
}

void TcpAsyncStream::handle_resolve(const std::error_code& error, tcp::resolver::results_type results, std::shared_ptr<tcp::resolver> resolver)
{
	if (!error && !results.empty())
	{
		TCP_LOG("resolved connecting");
		tcp::endpoint endpoint = results.begin()->endpoint();
		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, results, 1, resolver));
	}
	else
	{
		postFail("Resolve", error);
	}
}

void TcpAsyncStream::handle_resolver_connect(const std::error_code& error, tcp::resolver::results_type results, uint32_t counter, std::shared_ptr<tcp::resolver> resolver)
{
	if (error && results.size() < counter)
	{
		TCP_LOG("connect resolved next");

		asio::error_code ec;
		socket.close(ec);

		auto it = results.begin();
		for (uint32_t i = 0; i < counter; i++) it++;
		tcp::endpoint endpoint = it->endpoint();

		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, results, counter + 1, resolver));
	}
	else
	{
		if (!error)
		{
			TCP_LOG("resolved connected");
		}

		handle_connect(error);
	}
}

void TcpAsyncStream::do_close()
{
	postFail("Close", std::error_code());
}

void TcpAsyncStream::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	write_msgs.emplace_back(std::move(data));

	if (state == Connected)
	{
		bool writeInProgress = write_msgs.size() > 1;

		if (!writeInProgress)
		{
			TCP_LOG("writing " << write_msgs.front().size() << " bytes");

			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else if (state == Clear)
	{
		if (info.addressResolved)
		{
			connectByAddress();
		}
		else if (!info.host.empty())
		{
			connectByHostname();
		}
	}
}

void TcpAsyncStream::handle_write(const std::error_code& error)
{
	if (!error)
	{
		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		write_msgs.pop_front();

		if (!write_msgs.empty())
		{
			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

std::error_code TcpAsyncStream::readAvailableData()
{
	std::error_code ec;
	auto availableSize = uint32_t(socket.available(ec));
	if (ec)
		return ec;

	TCP_LOG("Socket available " << availableSize << " bytes");
	requestBandwidth(availableSize);

	if (availableSize > bw_quota) availableSize = bw_quota;
	if (availableSize > 0)
	{
		auto readData = readBuffer.reserve(availableSize);
		size_t bytes_transferred = socket.read_some(asio::buffer(readData, availableSize), ec);

		if (ec || bytes_transferred == 0)
		{
			if (ec != asio::error::would_block && ec != asio::error::try_again)
				return ec;
		}
		else
		{
			TCP_LOG("Read available " << bytes_transferred << " bytes");
			appendData(bytes_transferred);
		}
	}

	return {};
}

void TcpAsyncStream::handle_receive(const std::error_code& error, std::size_t bytes_transferred, std::size_t bytes_requested)
{
	TCP_LOG("received " << bytes_transferred << " bytes");
	waiting_for_data = false;

	if (state == Disconnected)
		return;

	if (!error)
	{
		std::error_code ec;

		{
			std::lock_guard<std::mutex> guard(receive_mutex);

			appendData(bytes_transferred);

			if (bytes_transferred == bytes_requested)
				ec = readAvailableData();

			if (!ec)
			{
				{
					std::lock_guard<std::mutex> guard(callbackMutex);

					if (onReceiveCallback)
					{
						size_t consumed = onReceiveCallback({ readBuffer.data.data(), readBuffer.pos });
						readBuffer.consume(consumed);
					}
				}

				startReceive();
			}
		}

		if (ec)
			postFail("Read", ec);
	}
	else
	{
		postFail("Receive", error);
	}
}

void TcpAsyncStream::appendData(size_t size)
{
	readBuffer.advanceBuffer(size);

	auto currentTimeDiff = (uint32_t)time(0) - lastReceiveTime;
	lastReceiveSpeed = (uint32_t)size / (currentTimeDiff * 1000);

	bw_quota = std::min(bw_quota - (uint32_t)size, (uint32_t)0);
}

void TcpAsyncStream::checkTimeout(const asio::error_code& error)
{
	if (state == Disconnected || error)
		return;

	postFail("timeout", std::error_code());
}

uint32_t TcpAsyncStream::wantedTransfer()
{
	uint32_t wanted_transfer = std::max({ expecting_size/*, m_recv_buffer.packet_bytes_remaining() + 30*/, lastReceiveSpeed });

	wanted_transfer = std::min(wanted_transfer, (uint32_t)1024*1024);

	TCP_LOG_DETAILED("wantedTransfer: " << wanted_transfer << ", expecting " << expecting_size << ", lastReceiveSpeed " << lastReceiveSpeed);

	return wanted_transfer;
}

void TcpAsyncStream::requestBandwidth(uint32_t bytes)
{
	if (waiting_for_bw)
		return;

	TCP_LOG_DETAILED("wants quota " << bytes << " bytes, current quota " << bw_quota << " bytes");

	if (bw_quota >= bytes)
		return;

	TCP_LOG_DETAILED("requesting " << bytes << " bytes");

	bytes -= bw_quota;

	uint32_t ret = bwChannelsCount ? BandwidthManager::Get().requestBandwidth(shared_from_this(), bytes, priority, bwChannels, bwChannelsCount) : bytes;
	TCP_LOG_DETAILED("request_bandwidth returned " << ret << " bytes");

	if (ret == 0)
		waiting_for_bw = true;
	else
		bw_quota += ret;
}

void TcpAsyncStream::startReceive()
{
	if (!isActive())
		return;

	uint32_t bytes = wantedTransfer();
	requestBandwidth(bytes);

	if (waiting_for_data || waiting_for_bw)
		return;

	uint32_t max_receive = bw_quota;

	waiting_for_data = true;

	timeoutTimer->expires_after(std::chrono::seconds(60));
	timeoutTimer->async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	TCP_LOG_DETAILED("call async_receive for " << max_receive << " bytes");

	auto readData = readBuffer.reserve(max_receive);
	socket.async_receive(asio::buffer(readData, max_receive), std::bind(&TcpAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2, max_receive));
}

bool TcpAsyncStream::isActive()
{
	return state == Connected;
}

void TcpAsyncStream::assignBandwidth(int amount)
{
	TCP_LOG_DETAILED("assignBandwidth returned " << amount << " bytes");

	std::lock_guard<std::mutex> guard(receive_mutex);

	waiting_for_bw = false;
	bw_quota += amount;

	startReceive();
}

void TcpAsyncStream::ReadBuffer::advanceBuffer(size_t size)
{
	pos += size;
	receivedCounter += size;
	TCP_LOG_DETAILED("Buffer advance " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());
}

void TcpAsyncStream::ReadBuffer::consume(size_t size)
{
	pos -= size;

	if (pos && size)
	{
		memmove(data.data(), data.data() + size, pos);
		data.resize(pos);
	}

	TCP_LOG_DETAILED("Buffer consume " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());
}

uint8_t* TcpAsyncStream::ReadBuffer::reserve(size_t size)
{
	if (pos && data.capacity() - pos < size)
	{
		auto tmp = std::move(data);
		data.resize(pos + size);
		memcpy(data.data(), tmp.data(), pos);
	}
	else
		data.resize(pos + size);

	TCP_LOG_DETAILED("Buffer reserve " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());

	return data.data() + pos;
}

size_t TcpAsyncStream::ReadBuffer::reserved()
{
	return data.size() - pos;
}

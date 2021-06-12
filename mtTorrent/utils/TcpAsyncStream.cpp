#include "TcpAsyncStream.h"
#include "Logging.h"

#define TCP_LOG(x) WRITE_LOG(LogTypeTcp, getHostname() << " " << x)
#define TCP_LOG_BUFFER(x) WRITE_LOG(LogTypeTcp, "Buffer " << std::to_string(reinterpret_cast<int>(this)) << " " << x)

TcpAsyncStream::TcpAsyncStream(asio::io_service& io) : io_service(io), socket(io), timeoutTimer(io)
{
	setBandwidthChannels(nullptr, 0);
}

TcpAsyncStream::~TcpAsyncStream()
{
}

void TcpAsyncStream::connect(const uint8_t* ip, uint16_t port, bool ipv6)
{
	if (state != Disconnected)
		return;

	info.address.set(ip, port, ipv6); 
	info.addressResolved = true;
	info.host = info.address.toString();

	timeoutTimer.expires_from_now(std::chrono::seconds(10));
	timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	connectByAddress();
}

void TcpAsyncStream::connect(const std::string& ip, uint16_t port)
{
	if (state != Disconnected)
		return;

	info.address.set(asio::ip::address::from_string(ip), port);
	info.addressResolved = true;
	info.host = ip;

	connectByAddress();
}

void TcpAsyncStream::init(const std::string& hostname, const std::string& port)
{
	info.host = hostname;
	info.address.port = (uint16_t)strtoul(port.data(), NULL, 10);
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

	io_service.post(std::bind(&TcpAsyncStream::do_close, shared_from_this()));
}

void TcpAsyncStream::write(const DataBuffer& data)
{
	io_service.post(std::bind(&TcpAsyncStream::do_write, shared_from_this(), data));
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

	tcp::resolver::query query(info.host, std::to_string(info.address.port));

	auto resolver = std::make_shared<tcp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&TcpAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void TcpAsyncStream::connectByAddress()
{
	TCP_LOG("connecting");

	state = Connecting;

	socket.async_connect(info.address.toTcpEndpoint(), std::bind(&TcpAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncStream::setAsConnected()
{
	state = Connected;

	auto endpoint = socket.remote_endpoint();
	info.address.set(endpoint.address(), endpoint.port());
	info.addressResolved = true;
	info.host = endpoint.address().to_string();

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
	if (state == Disconnected)
		return;

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

	if(error)
		TCP_LOG("error on " << place << ": " << error.message())
	else
		TCP_LOG("end on " << place);

	state = Disconnected;
	timeoutTimer.cancel();

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
		setAsConnected();
	}
	else
	{
		postFail("Connect", error);
	}
}

void TcpAsyncStream::handle_resolve(const std::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
{
	if (!error)
	{
		TCP_LOG("resolved connecting");
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, ++iterator, resolver));
	}
	else
	{
		postFail("Resolve", error);
	}
}

void TcpAsyncStream::handle_resolver_connect(const std::error_code& error, tcp::resolver::iterator iterator, std::shared_ptr<tcp::resolver> resolver)
{
	if (error && iterator != tcp::resolver::iterator())
	{
		TCP_LOG("connect resolved next");

		asio::error_code ec;
		socket.close(ec);
		tcp::endpoint endpoint = *iterator;
		socket.async_connect(endpoint, std::bind(&TcpAsyncStream::handle_resolver_connect, shared_from_this(), std::placeholders::_1, ++iterator, resolver));
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

	write_msgs.push_back(data);

	if (state == Connected)
	{
		TCP_LOG("writing " << data.size() << " bytes");

		bool write_in_progress = write_msgs.size() > 1;

		if (!write_in_progress)
		{
			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else if (state != Connecting)
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

bool TcpAsyncStream::readAvailableData()
{
	std::error_code ec;
	uint32_t availableSize = uint32_t(socket.available(ec));
	if (ec)
	{
		postFail("Available", ec);
		return false;
	}

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
			{
				postFail("Read", ec);
				return false;
			}
		}
		else
		{
			TCP_LOG("Read available " << bytes_transferred << " bytes");
			appendData(bytes_transferred);
		}
	}

	return true;
}

void TcpAsyncStream::handle_receive(const std::error_code& error, std::size_t bytes_transferred, std::size_t bytes_requested)
{
	TCP_LOG("received " << bytes_transferred << " bytes");
	waiting_for_data = false;

	if (!error)
	{
		std::lock_guard<std::mutex> guard(receive_mutex);

		appendData(bytes_transferred);

		if (bytes_transferred == bytes_requested && !readAvailableData())
			return;

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

	return wanted_transfer;
}

void TcpAsyncStream::requestBandwidth(uint32_t bytes)
{
	if (waiting_for_bw)
		return;

	TCP_LOG("wants quota " << bytes << " bytes, current quota " << bw_quota << " bytes");

	if (bw_quota >= bytes)
		return;

	TCP_LOG("requesting " << bytes << " bytes");

	bytes -= bw_quota;

	uint32_t ret = bwChannelsCount ? BandwidthManager::Get().requestBandwidth(shared_from_this(), bytes, priority, bwChannels, bwChannelsCount) : bytes;
	TCP_LOG("request_bandwidth returned " << ret << " bytes");

	if (ret == 0)
		waiting_for_bw = true;
	else
		bw_quota += ret;
}

void TcpAsyncStream::startReceive()
{
	uint32_t bytes = wantedTransfer();
	requestBandwidth(bytes);

	if (waiting_for_data || waiting_for_bw)
		return;

	uint32_t max_receive = bw_quota;

	waiting_for_data = true;

	timeoutTimer.expires_from_now(std::chrono::seconds(60));
	timeoutTimer.async_wait(std::bind(&TcpAsyncStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	TCP_LOG("call async_receive for " << max_receive << " bytes");

	auto readData = readBuffer.reserve(max_receive);
	socket.async_receive(asio::buffer(readData, max_receive), std::bind(&TcpAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2, max_receive));
}

bool TcpAsyncStream::isActive()
{
	return state == Connected;
}

void TcpAsyncStream::assignBandwidth(int amount)
{
	TCP_LOG("assignBandwidth returned " << amount << " bytes");

	std::lock_guard<std::mutex> guard(receive_mutex);

	waiting_for_bw = false;
	bw_quota += amount;

	if(isActive())
		startReceive();
}

void TcpAsyncStream::ReadBuffer::advanceBuffer(size_t size)
{
	pos += size;
	receivedCounter += size;
	TCP_LOG_BUFFER("advance " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());
}

void TcpAsyncStream::ReadBuffer::consume(size_t size)
{
	pos -= size;

	if (pos && size)
	{
		auto tmp = DataBuffer(data.begin() + size, data.begin() + size + pos);
		data.resize(pos);
		memcpy(data.data(), tmp.data(), pos);
	}

	TCP_LOG_BUFFER("consume " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());
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

	TCP_LOG_BUFFER("reserve " << size << " - Buffer pos " << pos << ", reserved " << reserved() << ", fullsize " << data.size());

	return data.data() + pos;
}

size_t TcpAsyncStream::ReadBuffer::reserved()
{
	return data.size() - pos;
}

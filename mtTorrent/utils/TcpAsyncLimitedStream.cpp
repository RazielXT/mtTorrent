#include "TcpAsyncLimitedStream.h"
#include "Logging.h"

#define TCP_LOG(x) WRITE_LOG(LogTypeTcp, getHostname() << " " << x)

TcpAsyncLimitedStream::TcpAsyncLimitedStream(asio::io_service& io) : io_service(io), socket(io), timeoutTimer(io)
{
	setBandwidthChannels(nullptr, 0);
}

TcpAsyncLimitedStream::~TcpAsyncLimitedStream()
{
}

void TcpAsyncLimitedStream::connect(const uint8_t* ip, uint16_t port, bool ipv6)
{
	if (state != Disconnected)
		return;

	info.endpoint = ipv6 ? tcp::endpoint(asio::ip::address_v6(*reinterpret_cast<const asio::ip::address_v6::bytes_type*>(ip)), port) :
		tcp::endpoint(asio::ip::address_v4(*reinterpret_cast<const asio::ip::address_v4::bytes_type*>(ip)), port);
	info.endpointInitialized = true;

	info.host = info.endpoint.address().to_string();
	info.port = port;

	timeoutTimer.expires_from_now(std::chrono::seconds(10));
	timeoutTimer.async_wait(std::bind(&TcpAsyncLimitedStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	connectEndpoint();
}

void TcpAsyncLimitedStream::close(bool immediate)
{
	if (immediate)
	{
		onConnectCallback = nullptr;
		onReceiveCallback = nullptr;
		onCloseCallback = nullptr;
	}

	if (state == Disconnected)
		return;

	io_service.post(std::bind(&TcpAsyncLimitedStream::do_close, shared_from_this()));
}

void TcpAsyncLimitedStream::write(const DataBuffer& data)
{
	io_service.post(std::bind(&TcpAsyncLimitedStream::do_write, shared_from_this(), data));
}

DataBuffer TcpAsyncLimitedStream::getReceivedData()
{
	std::lock_guard<std::mutex> guard(receive_mutex);

	return receiveBuffer;
}

void TcpAsyncLimitedStream::consumeData(size_t size)
{
	std::lock_guard<std::mutex> guard(receive_mutex);

	receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + size);
}

uint16_t TcpAsyncLimitedStream::getPort()
{
	return info.port;
}

std::string& TcpAsyncLimitedStream::getHostname()
{
	return info.host;
}

tcp::endpoint& TcpAsyncLimitedStream::getEndpoint()
{
	return info.endpoint;
}

size_t TcpAsyncLimitedStream::getReceivedDataCount()
{
	return receivedCounter;
}

void TcpAsyncLimitedStream::setBandwidthPriority(int p)
{
	priority = p;
}

void TcpAsyncLimitedStream::setMinBandwidthRequest(uint32_t size)
{
	expecting_size = size;
}

void TcpAsyncLimitedStream::setBandwidthChannels(BandwidthChannel** bwc, uint32_t count)
{
	for (uint32_t i = 0; i < 2; i++)
	{
		if (i < count)
			bwChannels[i] = bwc[i];
		else
			bwChannels[i] = nullptr;
	}
}

void TcpAsyncLimitedStream::connectEndpoint()
{
	TCP_LOG("connecting");

	state = Connecting;

	socket.async_connect(info.endpoint, std::bind(&TcpAsyncLimitedStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void TcpAsyncLimitedStream::setAsConnected()
{
	TCP_LOG("connected");

	state = Connected;
	info.endpoint = socket.remote_endpoint();
	info.endpointInitialized = true;

	std::error_code ec;
	socket.non_blocking(true, ec);
	if (ec)
	{
		postFail("non_blocking", ec);
		return;
	}

	startReceive();

	{
		std::lock_guard<std::mutex> guard(callbackMutex);

		if (onConnectCallback)
			onConnectCallback();
	}
}

void TcpAsyncLimitedStream::postFail(const char* place, const std::error_code& error)
{
	if (state == Disconnected)
		return;

	try
	{
		if (socket.is_open())
			socket.close();
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

void TcpAsyncLimitedStream::handle_connect(const std::error_code& error)
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

void TcpAsyncLimitedStream::do_close()
{
	postFail("Close", std::error_code());
}

void TcpAsyncLimitedStream::do_write(DataBuffer data)
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
				std::bind(&TcpAsyncLimitedStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else if (state != Connecting)
	{
		if (info.endpointInitialized)
		{
			connectEndpoint();
		}
	}
}

void TcpAsyncLimitedStream::handle_write(const std::error_code& error)
{
	if (!error)
	{
		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		write_msgs.pop_front();

		if (!write_msgs.empty())
		{
			asio::async_write(socket,
				asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				std::bind(&TcpAsyncLimitedStream::handle_write, shared_from_this(), std::placeholders::_1));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

bool TcpAsyncLimitedStream::readAvailableData()
{
	std::error_code ec;
	uint32_t availableSize = uint32_t(socket.available(ec));
	if (ec)
	{
		postFail("Available", ec);
		return false;
	}

	requestBandwidth(availableSize);

	if (availableSize > m_quota) availableSize = m_quota;
	if (availableSize > 0)
	{
		recv_buffer.resize(availableSize);
		size_t bytes_transferred = socket.read_some(asio::buffer(recv_buffer, availableSize), ec);

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
			appendData(recv_buffer.data(), bytes_transferred);
		}
	}

	return true;
}

void TcpAsyncLimitedStream::handle_receive(const std::error_code& error, std::size_t bytes_transferred)
{
	TCP_LOG("received " << bytes_transferred << " bytes");
	waiting_for_data = false;

	if (!error)
	{
		{
			std::lock_guard<std::mutex> guard(receive_mutex);

			appendData(recv_buffer.data(), bytes_transferred);

			if (int(bytes_transferred) == recv_buffer.size() && !readAvailableData())
				return;

			startReceive();
		}

		{
			std::lock_guard<std::mutex> guard(callbackMutex);

			if (onReceiveCallback)
				onReceiveCallback();
		}
	}
	else
	{
		postFail("Receive", error);
	}
}

void TcpAsyncLimitedStream::appendData(char* data, size_t size)
{
	receivedCounter += size;
	receiveBuffer.insert(receiveBuffer.end(), data, data + size);

	auto currentTimeDiff = (uint32_t)time(0) - lastReceiveTime;
	lastReceiveSpeed = (uint32_t)size / (currentTimeDiff * 1000);

	m_quota = std::min(m_quota - (uint32_t)size, (uint32_t)0);
}

void TcpAsyncLimitedStream::checkTimeout(const asio::error_code& error)
{
	if (state == Disconnected || error)
		return;

	postFail("timeout", std::error_code());
}

uint32_t TcpAsyncLimitedStream::wantedTransfer()
{
	uint32_t wanted_transfer = std::max({ expecting_size/*, m_recv_buffer.packet_bytes_remaining() + 30*/, lastReceiveSpeed });

	wanted_transfer = std::min(wanted_transfer, (uint32_t)1024*1024);

	return wanted_transfer;
}

void TcpAsyncLimitedStream::requestBandwidth(uint32_t bytes)
{
	if (waiting_for_bw)
		return;

	TCP_LOG("wants quota " << bytes << " bytes, current quota " << m_quota << " bytes");

	if (bytes == 0 || m_quota >= bytes)
		return;

	TCP_LOG("requesting " << bytes << " bytes");

	bytes -= m_quota;

	uint32_t ret = BandwidthManager::Get().requestBandwidth(shared_from_this(), bytes, priority, bwChannels, 1);
	TCP_LOG("request_bandwidth returned " << ret << " bytes");

	if (ret == 0)
		waiting_for_bw = true;
	else
		m_quota += ret;
}

void TcpAsyncLimitedStream::startReceive()
{
	if (recv_buffer.empty())
		recv_buffer.resize(100);

	uint32_t bytes = wantedTransfer();
	requestBandwidth(bytes);

	if (waiting_for_data || m_quota == 0)
		return;

	uint32_t max_receive = m_quota;// std::min(buffer_size, quota_left);

	waiting_for_data = true;

	timeoutTimer.expires_from_now(std::chrono::seconds(60));
	timeoutTimer.async_wait(std::bind(&TcpAsyncLimitedStream::checkTimeout, shared_from_this(), std::placeholders::_1));

	TCP_LOG("call async_receive for " << max_receive << " bytes");

	recv_buffer.resize(max_receive);
	socket.async_receive(asio::buffer(recv_buffer, max_receive), std::bind(&TcpAsyncLimitedStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

bool TcpAsyncLimitedStream::isActive()
{
	return state == Connected;
}

void TcpAsyncLimitedStream::assignBandwidth(int amount)
{
	TCP_LOG("assignBandwidth returned " << amount << " bytes");

	std::lock_guard<std::mutex> guard(receive_mutex);

	waiting_for_bw = false;
	m_quota += amount;

	if(isActive())
		startReceive();
}

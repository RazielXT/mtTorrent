#include "UdpAsyncClient.h"
#include "Logging.h"

#define UDP_LOG(x) WRITE_LOG("UDP: " << target_endpoint.address().to_string() << " " << x)

UdpAsyncClient::UdpAsyncClient(boost::asio::io_service& io) : io_service(io), socket(io), timeoutTimer(io)
{
}

UdpAsyncClient::~UdpAsyncClient()
{
}

void UdpAsyncClient::setAddress(Addr& addr)
{
	target_endpoint = addr.ipv6 ?
		udp::endpoint(boost::asio::ip::address_v6(*reinterpret_cast<boost::asio::ip::address_v6::bytes_type*>(addr.addrBytes)), addr.port) :
		udp::endpoint(boost::asio::ip::address_v4(*reinterpret_cast<boost::asio::ip::address_v4::bytes_type*>(addr.addrBytes)), addr.port);

	state = Initialized;
}

void UdpAsyncClient::setAddress(const std::string& hostname, const std::string& port)
{
	udp::resolver::query query(hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncClient::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncClient::setAddress(const std::string& hostname, const std::string& port, bool ipv6)
{
	udp::resolver::query query(ipv6 ? udp::v6() : udp::v4(), hostname, port);

	auto resolver = std::make_shared<udp::resolver>(io_service);
	resolver->async_resolve(query, std::bind(&UdpAsyncClient::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void UdpAsyncClient::close()
{
	onReceiveCallback = nullptr;
	onCloseCallback = nullptr;

	io_service.post(std::bind(&UdpAsyncClient::do_close, shared_from_this()));
}

bool UdpAsyncClient::write(const DataBuffer& data)
{
	if (listening)
		return false;

	timeoutTimer.async_wait(std::bind(&UdpAsyncClient::checkTimeout, shared_from_this()));

	io_service.post(std::bind(&UdpAsyncClient::do_write, this, data));

	return true;
}

void UdpAsyncClient::handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator, std::shared_ptr<udp::resolver> resolver)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		target_endpoint = *iterator;
		state = Initialized;

		if (!messageBuffer.empty())
			send_message();
	}
	else
	{
		postFail("Resolve", error);
	}
}

void UdpAsyncClient::listenToResponse()
{
	if (state != Connected)
		return;

	listening = true;
	responseBuffer.resize(2 * 1024);

	UDP_LOG("listening");

	timeoutTimer.expires_from_now(boost::posix_time::seconds(3));

	socket.async_receive_from(boost::asio::buffer(responseBuffer.data(), responseBuffer.size()), target_endpoint,
		std::bind(&UdpAsyncClient::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void UdpAsyncClient::postFail(std::string place, const boost::system::error_code& error)
{
	if(error)
		UDP_LOG(place << "-" << error.message())

	if(state == Connected)
		state = Initialized;

	if (onCloseCallback)
		onCloseCallback();
}

void UdpAsyncClient::handle_connect(const boost::system::error_code& error)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		state = Connected;
		send_message();
	}
	else
	{
		postFail("Connect", error);
	}
}

void UdpAsyncClient::do_close()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	onReceiveCallback = nullptr;
	onCloseCallback = nullptr;

	if (state == Connected)
		state = Initialized;

	listening = false;

	boost::system::error_code error;

	if (socket.is_open())
	{
		socket.shutdown(boost::asio::socket_base::shutdown_both);
		socket.close(error);
	}	
}

void UdpAsyncClient::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	messageBuffer = data;

	if (state != Clear)
		send_message();
}

void UdpAsyncClient::send_message()
{
	if (state == Initialized)
	{
		socket.async_connect(target_endpoint, std::bind(&UdpAsyncClient::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else if (state == Connected)
	{
		if (!messageBuffer.empty())
		{
			UDP_LOG("writing (try " << (int)writeRetries << ")");

			socket.async_send_to(boost::asio::buffer(messageBuffer.data(), messageBuffer.size()), target_endpoint,
				std::bind(&UdpAsyncClient::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}
}

void UdpAsyncClient::handle_write(const boost::system::error_code& error, size_t sz)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!error)
	{
		if(!listening)
			listenToResponse();
	}
	else
	{
		postFail("Write", error);
	}
}

void UdpAsyncClient::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	std::lock_guard<std::mutex> guard(stateMutex);

	listening = false;
	writeRetries = 0;
	UDP_LOG("received size: " << bytes_transferred);

	if (!error)
	{
		responseBuffer.resize(bytes_transferred);

		if (onReceiveCallback)
			onReceiveCallback(responseBuffer);
	}
}

void UdpAsyncClient::checkTimeout()
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (!listening)
		return;

	if (timeoutTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		if (writeRetries >= 1)
		{
			timeoutTimer.expires_at(boost::posix_time::pos_infin);
			postFail("timeout", boost::system::error_code());
		}
		else
		{
			writeRetries++;
			send_message();
		}
	}

	timeoutTimer.async_wait(std::bind(&UdpAsyncClient::checkTimeout, shared_from_this()));
}

UdpRequest SendAsyncUdp(Addr& addr, DataBuffer& data, boost::asio::io_service& io, std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult)
{
	UdpRequest req = std::make_shared<PackedUdpRequest>(io);
	req->client->setAddress(addr);
	req->write(data, onResult);

	return req;
}

UdpRequest SendAsyncUdp(const std::string& hostname, const std::string& port, bool ipv6, DataBuffer& data, boost::asio::io_service& io, std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult)
{
	UdpRequest req = std::make_shared<PackedUdpRequest>(io);
	req->write(data, onResult);
	req->client->setAddress(hostname, port, ipv6);

	return req;
}

UdpRequest CreateAsyncUdp(const std::string& hostname, const std::string& port, boost::asio::io_service& io)
{
	UdpRequest req = std::make_shared<PackedUdpRequest>(io);
	req->client->setAddress(hostname, port);

	return req;
}

PackedUdpRequest::PackedUdpRequest(boost::asio::io_service& io)
{
	client = std::make_shared<UdpAsyncClient>(io);
	auto ptr = client->shared_from_this();
}

PackedUdpRequest::~PackedUdpRequest()
{
	client->close();
}

void PackedUdpRequest::write(DataBuffer& data, std::function<void(DataBuffer* data, PackedUdpRequest* source)> onResult)
{
	this->onResult = onResult;
	client->onCloseCallback = std::bind(&PackedUdpRequest::onFail, this);
	client->onReceiveCallback = std::bind(&PackedUdpRequest::onSuccess, this, std::placeholders::_1);
	client->write(data);
}

void PackedUdpRequest::onFail()
{
	onResult(0, this);
}

void PackedUdpRequest::onSuccess(DataBuffer& response)
{
	onResult(&response, this);
}

void PackedUdpRequest::clear()
{
	client->onReceiveCallback = nullptr;
	client->onCloseCallback = nullptr;
}

#include "UdpAsyncClient.h"
#include <iostream>

UdpAsyncClient::UdpAsyncClient(boost::asio::io_service& io, Addr& addr) : io_service(io), socket(io)
{
	int32_t timeout = 15;
	setsockopt(socket.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(socket.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

	target_endpoint = addr.ipv6 ?
		udp::endpoint(boost::asio::ip::address_v6(*reinterpret_cast<boost::asio::ip::address_v6::bytes_type*>(addr.addrBytes)), addr.port) :
		udp::endpoint(boost::asio::ip::address_v4(*reinterpret_cast<boost::asio::ip::address_v4::bytes_type*>(addr.addrBytes)), addr.port);
}

void UdpAsyncClient::close()
{
	if (state != Disconnected)
		return;

	io_service.post(std::bind(&UdpAsyncClient::do_close, this));
}

void UdpAsyncClient::write(const DataBuffer& data)
{
	io_service.post(std::bind(&UdpAsyncClient::do_write, this, data));
}

void UdpAsyncClient::setAsConnected()
{
	if (state != Disconnected)
		return;

	state = Connected;

	socket.async_receive(boost::asio::buffer(recv_buffer),
		std::bind(&UdpAsyncClient::handle_receive, this,
			std::placeholders::_1,
			std::placeholders::_2));
}

void UdpAsyncClient::postFail(std::string place, const boost::system::error_code& error)
{
	if(error)
		std::cout << place << "-" << target_endpoint.address.to_string() << "-" << error.message() << "\n";

	state = Disconnected;

	if (onCloseCallback)
		onCloseCallback();
}

void UdpAsyncClient::do_close()
{
	boost::system::error_code error;

	if (socket.is_open())
		socket.close(error);

	postFail("Close", error);
}

void UdpAsyncClient::do_write(DataBuffer data)
{
	std::lock_guard<std::mutex> guard(write_msgs_mutex);

	bool write_in_progress = !write_msgs.empty();
	write_msgs.push_back(data);
	if (!write_in_progress)
	{
		socket.async_send_to(
			boost::asio::buffer(write_msgs.front().data(), write_msgs.front().size()), 
			target_endpoint, 
			std::bind(&UdpAsyncClient::handle_write, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void UdpAsyncClient::handle_write(const boost::system::error_code& error, size_t sz)
{
	if (!error)
	{
		if (state == Disconnected)
			state = Connected;

		std::lock_guard<std::mutex> guard(write_msgs_mutex);

		write_msgs.pop_front();
		if (!write_msgs.empty())
		{
			socket.async_send_to(
				boost::asio::buffer(write_msgs.front().data(), write_msgs.front().size()),
				target_endpoint,
				std::bind(&UdpAsyncClient::handle_write, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
	else
	{
		postFail("Write", error);
	}
}

void UdpAsyncClient::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		DataBuffer receivedData(recv_buffer.data(), recv_buffer.data() + bytes_transferred);

		socket.async_receive_from(boost::asio::buffer(recv_buffer), target_endpoint,
			std::bind(&UdpAsyncClient::handle_receive, this,
				std::placeholders::_1,
				std::placeholders::_2));

		if (onReceiveCallback)
			onReceiveCallback(receivedData);
	}
	else
	{
		postFail("Receive", error);
	}
}

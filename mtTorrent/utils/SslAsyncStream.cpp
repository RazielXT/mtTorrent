#include "SslAsyncStream.h"

#ifdef MTT_WITH_SSL

#define SSL_TCP_LOG(x) WRITE_LOG(x)

SslAsyncStream::SslAsyncStream(asio::io_context& io_context) : io(io_context), ctx(asio::ssl::context::tls), socket(io, ctx)
{
	CREATE_LOG(SslTcp);
	ctx.set_default_verify_paths();
}

SslAsyncStream::~SslAsyncStream()
{
	NAME_LOG(info.host);
}

void SslAsyncStream::init(const std::string& hostname, const std::string& service)
{
	info.host = hostname;
	info.service = service;

	SSL_set_tlsext_host_name(socket.native_handle(), hostname.data());
}

void SslAsyncStream::write(DataBuffer& data, std::function<void(const BufferView&)> onReceive)
{
	onReceiveCallback = onReceive;
	writeBuffer = std::move(data);

	do_write();
}

void SslAsyncStream::stop()
{
	onReceiveCallback = nullptr;
	writeBuffer.clear();

	if (socket.next_layer().is_open())
	{
		SSL_TCP_LOG("closing");
		asio::error_code ec;
		socket.next_layer().close(ec);
	}

	state = Disconnected;
}

void SslAsyncStream::connectByHostname()
{
	SSL_TCP_LOG("resolving hostname");

	state = Connecting;

	auto resolver = std::make_shared<tcp::resolver>(io);
	resolver->async_resolve(info.host, info.service, std::bind(&SslAsyncStream::handle_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2, resolver));
}

void SslAsyncStream::connectEndpoint()
{
	SSL_TCP_LOG("connecting");

	state = Connecting;

	socket.next_layer().async_connect(info.endpoint, std::bind(&SslAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
}

void SslAsyncStream::postFail(const char* place, const std::error_code& error)
{
	if (state == Disconnected)
		return;

	if (error)
		SSL_TCP_LOG("error on " << place << ": " << error.message())
	else
		SSL_TCP_LOG("end on " << place);

	state = Disconnected;

	if (onReceiveCallback)
		onReceiveCallback({0, 0});

	onReceiveCallback = nullptr;
}

void SslAsyncStream::handle_resolve(const std::error_code& error, tcp::resolver::results_type results, std::shared_ptr<tcp::resolver> resolver)
{
	if (!error && !results.empty())
	{
		tcp::endpoint endpoint = results.begin()->endpoint();
		SSL_TCP_LOG("resolved connecting " << endpoint.address().to_string());
		socket.next_layer().async_connect(endpoint, std::bind(&SslAsyncStream::handle_connect, shared_from_this(), std::placeholders::_1));
	}
	else
	{
		postFail("Resolve", error);
	}
}

void SslAsyncStream::handle_connect(const std::error_code& error)
{
	if (!error)
	{
		state = Connected;
		info.endpoint = socket.next_layer().remote_endpoint();
		info.endpointInitialized = true;

		socket.lowest_layer().set_option(tcp::no_delay(true));
		socket.set_verify_mode(asio::ssl::verify_none);
		socket.async_handshake(asio::ssl::stream_base::client, std::bind(&SslAsyncStream::handle_handshake, shared_from_this(), std::placeholders::_1));
	}
	else
	{
		postFail("Connect", error);
	}
}

void SslAsyncStream::handle_handshake(const asio::error_code& error)
{
	if (!error)
	{
		do_write();
	}
	else
	{
		postFail("Handshake", error);
	}
}

void SslAsyncStream::do_write()
{
	if (state == Connected)
	{
		SSL_TCP_LOG("writing " << writeBuffer.size() << " bytes");
		socket.async_write_some(asio::buffer(writeBuffer.data(), writeBuffer.size()), std::bind(&SslAsyncStream::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
	else if (state != Connecting)
	{
		if (info.endpointInitialized)
		{
			connectEndpoint();
		}
		else if (!info.host.empty())
		{
			connectByHostname();
		}
	}
}

void SslAsyncStream::handle_write(asio::error_code ec, std::size_t)
{
	if (ec)
		postFail("Write", ec);
	else
	{
		readBufferTmp.resize(512);
		socket.async_read_some(asio::buffer(readBufferTmp.data(), readBufferTmp.size()), std::bind(&SslAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2, readBufferTmp.size()));
	}
}

void SslAsyncStream::handle_receive(asio::error_code ec, std::size_t received, std::size_t bytes_requested)
{
	if (ec)
		postFail("Receive", ec);
	else
	{
		if (received)
			readBuffer.insert(readBuffer.end(), readBufferTmp.begin(), readBufferTmp.begin() + received);

		SSL_TCP_LOG("Received " << received << " bytes");

		if (received == bytes_requested)
		{
			socket.async_read_some(asio::buffer(readBufferTmp.data(), readBufferTmp.size()), std::bind(&SslAsyncStream::handle_receive, shared_from_this(), std::placeholders::_1, std::placeholders::_2, readBufferTmp.size()));
		}
		else if (!readBuffer.empty() && onReceiveCallback)
		{
			SSL_TCP_LOG("Returning in total " << readBuffer.size() << " bytes");
			auto tmp = std::move(readBuffer);
			onReceiveCallback({ tmp.data(), tmp.size() });
		}
	}
}

#endif
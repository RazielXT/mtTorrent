#include "PeerStream.h"
#include "Api/Configuration.h"

mtt::PeerStream::PeerStream(asio::io_service& io) : io_service(io)
{

}

mtt::PeerStream::~PeerStream()
{

}

void mtt::PeerStream::fromStream(std::shared_ptr<TcpAsyncStream> stream)
{
	tcpStream = stream;
	initializeTcpStream();

	onOpenCallback();
}

void mtt::PeerStream::fromStream(utp::StreamPtr stream)
{
	utpStream = stream;
	initializeUtpStream();

	onOpenCallback();
}

void mtt::PeerStream::open(const Addr& address)
{
	if (mtt::config::getExternal().transfer.utp.enabled)
		openUtpStream(address);
	else
		openTcpStream(address);
}

void mtt::PeerStream::write(const DataBuffer& data)
{
	if (utpStream)
		utpStream->write(data);
	else if (tcpStream)
		tcpStream->write(data);
}

void mtt::PeerStream::close()
{
	if (tcpStream)
		tcpStream->close(false);
}

Addr mtt::PeerStream::getAddress() const
{
	if (tcpStream)
		return tcpStream->getAddress();
	if (utpStream)
		return utpStream->getAddress();

	return Addr::Empty();
}

std::string mtt::PeerStream::getAddressName() const
{
	if (tcpStream)
		return tcpStream->getHostname();
	if (utpStream)
		return utpStream->getHostname();

	return {};
}

uint64_t mtt::PeerStream::getReceivedDataCount() const
{
	if (tcpStream)
		return tcpStream->getReceivedDataCount();
	if (utpStream)
		return utpStream->getReceivedDataCount();

	return 0;
}

void mtt::PeerStream::setMinBandwidthRequest(uint32_t size)
{
	if (tcpStream)
		tcpStream->setMinBandwidthRequest(size);
}

void mtt::PeerStream::initializeTcpStream()
{
	tcpStream = std::make_shared<TcpAsyncStream>(io_service);

	tcpStream->onConnectCallback = std::bind(&PeerStream::connectionOpened, shared_from_this(), Type::Tcp);
	tcpStream->onCloseCallback = [this](int code) { connectionClosed(Type::Tcp, code); };
	tcpStream->onReceiveCallback = [this](const BufferView& buffer) { return dataReceived(Type::Tcp, buffer); };

	BandwidthChannel* channels[2];
	channels[0] = BandwidthManager::Get().GetChannel("");
	channels[1] = nullptr;//BandwidthManager::Get().GetChannel(hexToString(torrent.hash, 20));
	tcpStream->setBandwidthChannels(channels, 2);
}

void mtt::PeerStream::openTcpStream(const Addr& address)
{
	initializeTcpStream();
	tcpStream->connect(address.addrBytes, address.port, address.ipv6);
}

void mtt::PeerStream::initializeUtpStream()
{
	utpStream->onCloseCallback = std::bind(&PeerStream::connectionClosed, shared_from_this(), Type::Utp, std::placeholders::_1);
	utpStream->onReceiveCallback = [this](const BufferView& buffer) { return dataReceived(Type::Utp, buffer); };
}

void mtt::PeerStream::openUtpStream(const Addr& address)
{
	utpStream = utp::Manager::get().createStream(address.toUdpEndpoint(), [this, address](bool success)
		{
			if (success)
			{
				initializeUtpStream();
				connectionOpened(Type::Utp);
			}
			else
			{
				openTcpStream(address);
			}
		});
}

void mtt::PeerStream::connectionOpened(Type t)
{
	if (t != Type::Utp)
		utpStream = nullptr;

	return onOpenCallback();
}

void mtt::PeerStream::connectionClosed(Type, int code)
{
	return onCloseCallback(code);
}

size_t mtt::PeerStream::dataReceived(Type, const BufferView& buffer)
{
	return onReceiveCallback(buffer);
}

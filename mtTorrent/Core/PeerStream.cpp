#include "PeerStream.h"

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

	onOpenCallback();
}

void mtt::PeerStream::open(const Addr& address)
{
	// 	utpStream = utp::Manager::get().createStream(address.toUdpEndpoint(), [this, address](bool success)
	// 		{
	// 			if (success)
	// 			{
	// 				ext.utpStream = utpStream;
	// 				utpStream->onCloseCallback = [this](int code) {connectionClosed(code); };
	// 				utpStream->onReceiveCallback = std::bind(&PeerCommunication::dataReceived, shared_from_this(), std::placeholders::_1);
	// 				connectionOpened();
	// 			}
	// 			else
	// 			{
					initializeTcpStream();
					tcpStream->connect(address.addrBytes, address.port, address.ipv6);
	// 			}
	// 		});
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

const Addr& mtt::PeerStream::getAddress() const
{
	return tcpStream->getAddress();
}

const std::string& mtt::PeerStream::getAddressName() const
{
	return tcpStream->getHostname();
}

uint64_t mtt::PeerStream::getReceivedDataCount() const
{
	return tcpStream->getReceivedDataCount();
}

void mtt::PeerStream::setMinBandwidthRequest(uint32_t size)
{
	if (tcpStream)
		tcpStream->setMinBandwidthRequest(size);
}

void mtt::PeerStream::initializeTcpStream()
{
	if (!tcpStream)
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
}

void mtt::PeerStream::connectionOpened(Type)
{
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

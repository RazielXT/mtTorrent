#include "PeerStream.h"
#include "Api/Configuration.h"
#include "Api/Interface.h"

mtt::PeerStream::PeerStream(asio::io_service& io) : io_service(io)
{
}

mtt::PeerStream::~PeerStream()
{
}

void mtt::PeerStream::fromStream(std::shared_ptr<TcpAsyncStream> stream)
{
	tcpStream = stream;
	state.remoteConnection = true;
	initializeTcpStream();

	connectionOpened(Type::Tcp);
}

void mtt::PeerStream::fromStream(utp::StreamPtr stream)
{
	utpStream = stream;
	state.remoteConnection = true;
	initializeUtpStream();

	connectionOpened(Type::Utp);
}

void mtt::PeerStream::addEncryption(std::unique_ptr<ProtocolEncryption> encryption)
{
	pe = std::move(encryption);
}

void mtt::PeerStream::open(const Addr& address, const uint8_t* t)
{
	infoHash = t;

	auto& c = mtt::config::getExternal().connection;

	if (c.enableUtpOut && (c.preferUtp || !c.enableTcpOut))
		openUtpStream(address);
	else if (c.enableTcpOut)
		openTcpStream(address);
	else
	{
		auto ptr = shared_from_this();
		io_service.post([ptr] { ptr->onCloseCallback(0); });
	}
}

void mtt::PeerStream::write(DataBuffer data)
{
	if (!state.remoteConnection)
	{
		if (state.firstWrite && !peHandshake)
		{
			initialMessage = data;

			if (config::getExternal().connection.encryption == config::Encryption::Require)
			{
				startProtocolEncryption(data);
				return;
			}
		}

		state.firstWrite = false;
	}

	if (pe)
	{
		pe->encrypt(data.data(), data.size());
	}

	if (tcpStream)
		tcpStream->write(std::move(data));
	else if (utpStream)
		utpStream->write(data);
}

void mtt::PeerStream::close()
{
	state.manualClose = true;

	if (tcpStream)
		tcpStream->close(false);
	else if (utpStream)
		utpStream->close();
}

Addr mtt::PeerStream::getAddress() const
{
	if (tcpStream)
		return tcpStream->getAddress();
	if (utpStream)
		return utpStream->getAddress();

	return {};
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

uint32_t mtt::PeerStream::getFlags() const
{
	uint32_t f = 0;

	if (tcpStream)
		f |= PeerFlags::Tcp;
	if (utpStream)
		f |= PeerFlags::Utp;
	if (state.remoteConnection)
		f |= PeerFlags::RemoteConnection;
	if (pe)
		f |= PeerFlags::Encrypted;

	return f;
}

void mtt::PeerStream::initializeTcpStream()
{
	if (!tcpStream || state.reconnect)
		tcpStream = std::make_shared<TcpAsyncStream>(io_service);

	tcpStream->onConnectCallback = std::bind(&PeerStream::connectionOpened, shared_from_this(), Type::Tcp);
	tcpStream->onCloseCallback = [this](int code) { connectionClosed(Type::Tcp, code); };
	tcpStream->onReceiveCallback = [this](BufferSpan buffer) { return dataReceived(Type::Tcp, buffer); };

	BandwidthChannel* channels[2];
	channels[0] = BandwidthManager::Get().GetChannel("");
	channels[1] = nullptr;//BandwidthManager::Get().GetChannel(hexToString(torrent.hash, 20));
	tcpStream->setBandwidthChannels(channels, 2);

	state.tcpTried = true;
}

void mtt::PeerStream::openTcpStream(const Addr& address)
{
	initializeTcpStream();
	tcpStream->connect(address.addrBytes, address.port, address.ipv6);
}

void mtt::PeerStream::initializeUtpStream()
{
	if (!utpStream || state.reconnect)
		utpStream = std::make_shared<mtt::utp::Stream>(io_service);

	utpStream->onConnectCallback = std::bind(&PeerStream::connectionOpened, shared_from_this(), Type::Utp);
	utpStream->onCloseCallback = [this](int code) { connectionClosed(Type::Utp, code); };
	utpStream->onReceiveCallback = [this](BufferSpan buffer) { return dataReceived(Type::Utp, buffer); };

	state.utpTried = true;
}

void mtt::PeerStream::openUtpStream(const Addr& address)
{
	initializeUtpStream();
	utp::Manager::get().connectStream(utpStream, address.toUdpEndpoint());
}

void mtt::PeerStream::reconnectStream()
{
	state.reconnect = true;

	auto addr = getAddress();

	if (tcpStream)
		openTcpStream(addr);
	else if (utpStream)
		openUtpStream(addr);
}

void mtt::PeerStream::connectionOpened(Type t)
{
	state.connected = true;

	if (state.reconnect)
		sendEncryptionMethodRetry();
	else if (onOpenCallback)
		onOpenCallback();
}

void mtt::PeerStream::connectionClosed(Type t, int code)
{
	if (!state.remoteConnection)
	{
		if (!state.connected && !state.manualClose)
		{
			if (t == Type::Utp && !state.tcpTried && mtt::config::getExternal().connection.enableTcpOut)
			{
				openTcpStream(utpStream->getAddress());
				utpStream = nullptr;
				return;
			}

			if (t == Type::Tcp && !state.utpTried && mtt::config::getExternal().connection.enableUtpOut)
			{
				openUtpStream(tcpStream->getAddress());
				tcpStream = nullptr;
				return;
			}
		}

		if (state.connected && retryEncryptionMethod())
		{
			return;
		}
	}

	auto ptr = shared_from_this();

	onCloseCallback(code);

	//clean shared_from_this bind
	if (utpStream)
		utpStream->onConnectCallback = nullptr;
	if (tcpStream)
		tcpStream->onConnectCallback = nullptr;
}

size_t mtt::PeerStream::dataReceived(Type t, BufferSpan buffer)
{
	if (peHandshake)
		return dataReceivedPeHandshake(t, buffer);

	if (pe)
		pe->decrypt(buffer.getOffset(lastUnhandledDataSize));

	initialMessage.clear();

	auto sz = onReceiveCallback(buffer);

	lastUnhandledDataSize = buffer.size - sz;

	return sz;
}

size_t mtt::PeerStream::dataReceivedPeHandshake(Type t, BufferSpan buffer)
{
	DataBuffer response;
	auto sz = peHandshake->readRemoteDataAsInitiator(buffer, response);

	if (peHandshake->failed())
	{
		close();
		return sz;
	}

	if (!response.empty())
		write(response);

	if (peHandshake->established())
	{
		if (peHandshake->type != ProtocolEncryptionHandshake::Type::PlainText)
			pe = std::move(peHandshake->pe);

		peHandshake.reset();

		if (buffer.size > sz)
			sz += dataReceived(t, buffer.getOffset(sz));
	}

	return sz;
}

void mtt::PeerStream::startProtocolEncryption(const DataBuffer& data)
{
	peHandshake = std::make_unique<ProtocolEncryptionHandshake>();
	auto start = peHandshake->initiate(infoHash, data);
	write(start);
}

bool mtt::PeerStream::retryEncryptionMethod()
{
	auto data = std::move(initialMessage);

	//unfinished encrypted handshake
	if (peHandshake)
	{
		if (!peHandshake->failed() && config::getExternal().connection.encryption != config::Encryption::Require)
			initialMessage = std::move(data);
	}
	//unencrypted handshake without response
	else if (getReceivedDataCount() == 0 && config::getExternal().connection.encryption != config::Encryption::Refuse)
	{
		initialMessage = std::move(data);
	}

	if (!initialMessage.empty())
		reconnectStream();

	return !initialMessage.empty();
}

void mtt::PeerStream::sendEncryptionMethodRetry()
{
	DataBuffer retry{ std::move(initialMessage) };

	//switch
	if (peHandshake)
	{
		peHandshake.reset();
		write(retry);
	}
	else
	{
		startProtocolEncryption(retry);
	}
}

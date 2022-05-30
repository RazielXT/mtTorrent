#pragma once

#include "utils/TcpAsyncStream.h"
#include "Utp/UtpManager.h"
#include "ProtocolEncryption.h"
#include <optional>

namespace mtt
{
	class PeerStream : public std::enable_shared_from_this<PeerStream>
	{
	public:

		PeerStream(asio::io_service& io);
		~PeerStream();

		void fromStream(std::shared_ptr<TcpAsyncStream> stream);
		void fromStream(utp::StreamPtr stream);
		void addEncryption(std::unique_ptr<ProtocolEncryption> pe);

		std::function<void()> onOpenCallback;
		std::function<void(int)> onCloseCallback;
		std::function<size_t(BufferSpan)> onReceiveCallback;

		void open(const Addr& address, const uint8_t* infoHash);
		void write(DataBuffer);
		void close();

		Addr getAddress() const;
		std::string getAddressName() const;

		uint64_t getReceivedDataCount() const;

		void setMinBandwidthRequest(uint32_t size);

		uint32_t getFlags() const;

	protected:

		asio::io_service& io_service;

		std::shared_ptr<TcpAsyncStream> tcpStream;
		void initializeTcpStream();
		void openTcpStream(const Addr& address);

		utp::StreamPtr utpStream;
		void initializeUtpStream();
		void openUtpStream(const Addr& address);

		void reconnectStream();
		void closeStream();

		void startProtocolEncryption(const DataBuffer&);

		bool retryEncryptionMethod();
		DataBuffer initialMessage;
		void sendEncryptionMethodRetry();

		enum class Type
		{
			Tcp,
			Utp
		};

		void connectionOpened(Type);
		void connectionClosed(Type, int);
		size_t dataReceived(Type, BufferSpan);
		size_t dataReceivedPeHandshake(Type, BufferSpan);

		std::unique_ptr<ProtocolEncryptionHandshake> peHandshake;

		std::unique_ptr<ProtocolEncryption> pe;
		size_t lastUnhandledDataSize = 0;

		struct
		{
			bool manualClose = false;
			bool connected = false;
			bool remoteConnection = false;
			bool tcpTried = false;
			bool utpTried = false;
			bool reconnect = false;
			bool firstWrite = true;
		}
		state;

		const uint8_t* infoHash;

		FileLog log;
	};
}
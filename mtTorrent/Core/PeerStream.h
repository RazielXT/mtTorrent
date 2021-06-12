#pragma once

#include "utils/TcpAsyncStream.h"
#include "Utp/UtpManager.h"

namespace mtt
{
	class PeerStream : public std::enable_shared_from_this<PeerStream>
	{
	public:

		PeerStream(asio::io_service& io);
		~PeerStream();

		void fromStream(std::shared_ptr<TcpAsyncStream> stream);
		void fromStream(utp::StreamPtr stream);

		std::function<void()> onOpenCallback;
		std::function<void(int)> onCloseCallback;
		std::function<size_t(const BufferView&)> onReceiveCallback;

		void open(const Addr& address);
		void write(const DataBuffer&);
		void close();

		const Addr& getAddress() const;
		const std::string& getAddressName() const;

		uint64_t getReceivedDataCount() const;

		void setMinBandwidthRequest(uint32_t size);

	protected:

		asio::io_service& io_service;

		std::shared_ptr<TcpAsyncStream> tcpStream;
		utp::StreamPtr utpStream;

		void initializeTcpStream();

		enum class Type
		{
			Tcp,
			Utp
		};

		void connectionOpened(Type);
		void connectionClosed(Type, int);
		size_t dataReceived(Type, const BufferView&);
	};
}
#pragma once

#include "UtpStream.h"
#include "utils/ServiceThreadpool.h"
#include "utils/ScheduledTimer.h"
#include <map>
#include <chrono>
#include "utils/UdpAsyncReceiver.h"


namespace mtt
{
	namespace utp
	{
		using StreamPtr = std::shared_ptr<Stream>;

		class Manager
		{
		public:

			Manager();
			~Manager();

			static Manager& get();

			void start(uint16_t port);
			void stop();

			StreamPtr createStream(udp::endpoint& e, /*asio::io_service& io_service,*/ std::function<void(bool)> onResult);

			bool onUdpPacket(udp::endpoint&, std::vector<DataBuffer*>&);

			StreamPtr getStream() { return streams.begin()->second; }

			std::function<void(StreamPtr)> onConnection;

		private:

			void refresh();
			void onNewConnection(const udp::endpoint& e, const MessageHeader&);

			std::mutex streamsMutex;
			std::multimap<uint16_t, StreamPtr> streams;

			ServiceThreadpool service;
			bool active = false;

			std::vector<uint32_t> headerSizes;
			std::vector<DataBuffer*> usedBuffers;

			uint16_t currentUdpPort = {};

			std::shared_ptr<ScheduledTimer> timeoutTimer;
		};
	};
};

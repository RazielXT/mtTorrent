#pragma once

#include "UtpStream.h"
#include "utils/ServiceThreadpool.h"
#include "utils/ScheduledTimer.h"
#include "utils/UdpAsyncReceiver.h"
#include <map>
#include <chrono>

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

			StreamPtr createStream(const udp::endpoint& e, std::function<void(bool)> onResult);

			StreamPtr getStream() { return streams.begin()->second; }

			std::function<void(StreamPtr)> onConnection;

			bool onUdpPacket(udp::endpoint&, std::vector<DataBuffer*>&);

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

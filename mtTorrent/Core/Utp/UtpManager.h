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

			void init();
			void start();
			void stop();

			void connectStream(StreamPtr s, const udp::endpoint& e);

			void setConnectionCallback(std::function<void(StreamPtr)>);

			bool onUdpPacket(udp::endpoint&, std::vector<DataBuffer*>&);

		private:

			void refresh();
			void onNewConnection(const udp::endpoint& e, const MessageHeader&);

			std::mutex callbackMutex;
			std::function<void(StreamPtr)> onConnection;

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

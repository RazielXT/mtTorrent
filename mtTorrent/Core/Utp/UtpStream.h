#pragma once

#include "Logging.h"
#include "utils/UdpAsyncWriter.h"
#include "utils/ByteSwap.h"
#include <mutex>
#include <chrono>

namespace mtt
{
	namespace utp
	{
		using TimeClock = std::chrono::high_resolution_clock;
		using TimePoint = TimeClock::time_point;
		struct MessageHeader;
		struct ExtensionMessage;
		class Manager;
		enum MessageType { ST_DATA, ST_FIN, ST_STATE, ST_RESET, ST_SYN };

		class Stream : public std::enable_shared_from_this<Stream>//, public BandwidthUser
		{
		public:

			Stream(asio::io_service& io_service);
			~Stream();

			void init(const udp::endpoint&, uint16_t bindPort);

			void connect();
			void connectRemote(const MessageHeader& header);

			std::function<void()> onConnectCallback;
			std::function<size_t(BufferView)> onReceiveCallback;
			std::function<void(int)> onCloseCallback;

			void write(const DataBuffer&);

			bool readUdpPacket(const MessageHeader& header, const BufferView&);
			void readFinish();

			bool refresh(TimePoint now);

			uint16_t getId();
			const udp::endpoint& getEndpoint() const;

			std::string getHostname() const;
			Addr getAddress() const;
			uint64_t getReceivedDataCount() const;
			bool wasConnected() const;

			void close();

		private:

			void sendSyn();
			void sendState();
			void sendFin();
			void prepareStateHeader(MessageType);
			uint8_t getSelectiveAckDataSize();
			void prepareSelectiveAckData(uint8_t* data, uint8_t size);
			void flushSendData();

			bool updateState(const MessageHeader& msg);

			void handleStateMessage(const MessageHeader& msg);
			void handleDataMessage(const MessageHeader& msg, const BufferView& data);
			void handleFinMessage(const MessageHeader& msg);
			void handleResetMessage(const MessageHeader& msg);
			void handleExtension(const MessageHeader& msg, const ExtensionMessage& ext, uint8_t type);

			enum class StateType { CLEAN, SYN_SEND, SYN_RECV, CONNECTED, CLOSING, CLOSED };
			struct
			{
				StateType step = StateType::CLEAN;

				uint16_t sequence = 100;
				uint16_t ack = 0;

				uint16_t id_recv = {};
				uint16_t id_send = {};

				std::mutex mutex;
			}
			state;

			struct
			{
				uint32_t peer_wnd_size = 0x100000;

				uint32_t last_reply_delay = 0;

				//rtt
				//rtt_var

				uint16_t last_ack = 0;
				uint16_t eof_ack = 0;

				uint32_t timeoutCount = 0;
				TimePoint nextTimeout;
				TimePoint lastReceive;
				bool wasConnected = false;
			}
			connection;
			
			uint32_t packetTimeout() const;

			struct Sending
			{
				DataBuffer stateBuffer;

				struct SentDataPacket
				{
					TimePoint sendTime;
					uint16_t bufferIdx;
					uint16_t sequence;
					bool resent = false;
					bool needResend = false;
				};
				std::vector<SentDataPacket> sentData;
				size_t sentDataWithoutAckSize = 0;
				uint32_t duplicateAckCounter = 0;

				struct
				{
					DataBuffer buffer;
					DataBuffer buffer2;
					uint32_t bufferPos = 0;
				}
				prepared;

				struct
				{
					const uint32_t Min = 576;
					const uint32_t Max = 1500 - 20 - 8 - 20;	//max packet - (ip+udp+utp headers)
					uint32_t workingMaxSize = Min;

					uint16_t probeId = 0;
					bool probeSendingFinished = false;
				}
				mtu;
			}
			sending;

			std::shared_ptr<UdpAsyncWriter> writer;
			uint32_t sendData(const uint8_t* data, uint32_t dataSize);
			void ackSentDataBuffer(const MessageHeader& header);
			void ackSentDataBuffer(const MessageHeader& header, const uint8_t* selectiveAcks, uint8_t len);
			void ackSentPacket(const MessageHeader& header, const Sending::SentDataPacket& packet);
			void resendPackets(uint16_t lastIdx);
			void clearSendData();
			void checkEof();

			struct 
			{
				const uint32_t wnd_size = 1024 * 1024;
				DataBuffer receiveBuffer;

				struct ReceivedDataUnsynced
				{
					uint16_t bufferIdx;
					uint16_t sequence;
				};
				std::vector<ReceivedDataUnsynced> receivedData;

				struct 
				{
					bool received = false;
					bool appended = false;
				}
				state;

				uint64_t bytesSize = 0;
			}
			receiving;

			void flushReceivedData();
			void appendReceiveBuffer(const BufferView& data);
			void storeReceivedData(uint16_t sequenceId, const BufferView& data);
			uint32_t getReceiveWindow();

			std::pair<DataBuffer&, uint16_t> createDataBuffer(size_t size);
			std::vector<DataBuffer> dataBuffers;//cache for reuse

			asio::io_service& io_service;

			std::mutex callbackMutex;

			FileLog log;
		};

		template <class T, T(*F)(T) >
		struct BigEndian
		{
			BigEndian& operator=(T v)
			{
				data = F(v);
				return *this;
			}
			operator T() const
			{
				return F(data);
			}
		private:
			T data;
		};

#ifdef _WIN32
		using be_uint32 = BigEndian<unsigned long, swap32>;
#else
		using be_uint32 = BigEndian<uint32_t, swap32>;
#endif

		using be_uint16 = BigEndian<std::uint16_t, swap16>;

		struct MessageHeader
		{
			static const uint8_t CurrentVersion = 1;
			void setType(MessageType type) { type_ver = (type << 4) | CurrentVersion; };
			MessageType getType() const { return MessageType(type_ver >> 4); };
			uint8_t getVersion() const { return type_ver & 0xF; };

			uint8_t type_ver;
			uint8_t extension;
			be_uint16 connection_id;

			be_uint32 timestamp_microseconds;
			be_uint32 timestamp_difference_microseconds;
			be_uint32 wnd_size;

			be_uint16 seq_nr;
			be_uint16 ack_nr;
		};

		uint32_t parseHeaderSize(const MessageHeader* header, std::size_t dataSize);

		struct ExtensionMessage
		{
			uint8_t nextExtensionType;
			uint8_t len;

			static const uint8_t SelectiveAcksExtensionType = 1;
		};

		uint32_t timestampMicro();
		uint32_t timestampMicro(const TimePoint& now);
	};
};

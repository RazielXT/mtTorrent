#include "UtpStream.h"
#include "Api/Configuration.h"
#include "UtpManager.h"
#include "Logging.h"
#include "utils/HexEncoding.h"

#define UTP_LOG(x) WRITE_LOG("UtpStream", x)

constexpr uint16_t MTU_PROBE_IDLE_ID = 0;
constexpr uint32_t MaxDuplicateAckBeforeResend = 2;
constexpr uint32_t MaxStreamTimeout = 30000;

//handle 16 bit sequence overflow comparison
static bool idLowerThan(std::uint32_t lhs, std::uint32_t rhs)
{
	std::uint32_t dist_down = (lhs - rhs) & 0xFFFF;
	std::uint32_t dist_up = (rhs - lhs) & 0xFFFF;

	return dist_up < dist_down;
}
static bool idHigherThan(std::uint32_t lhs, std::uint32_t rhs)
{
	std::uint32_t dist_down = (lhs - rhs) & 0xFFFF;
	std::uint32_t dist_up = (rhs - lhs) & 0xFFFF;

	return dist_up > dist_down;
}

template <typename Int, Int inverted_gain>
struct sliding_average
{
	sliding_average() : m_mean(0), m_average_deviation(0), m_num_samples(0) {}

	void add_sample(Int s)
	{
		// fixed point
		s *= 64;
		Int const deviation = (m_num_samples > 0) ? std::abs(m_mean - s) : 0;

		if (m_num_samples < inverted_gain)
			++m_num_samples;

		m_mean += (s - m_mean) / m_num_samples;

		if (m_num_samples > 1) {
			// the exact same thing for deviation off the mean except -1 on
			// the samples, because the number of deviation samples always lags
			// behind by 1 (you need to actual samples to have a single deviation
			// sample).
			m_average_deviation += (deviation - m_average_deviation) / (m_num_samples - 1);
		}
	}

	Int mean() const { return m_num_samples > 0 ? (m_mean + 32) / 64 : 0; }
	Int avg_deviation() const { return m_num_samples > 1 ? (m_average_deviation + 32) / 64 : 0; }
	int num_samples() const { return m_num_samples; }

private:
	// both of these are fixed point values (* 64)
	Int m_mean = 0;
	Int m_average_deviation = 0;
	// the number of samples we have received, but no more than inverted_gain
	// this is the effective inverted_gain
	int m_num_samples = 0;
};
sliding_average<int, 16> m_rtt;

mtt::utp::Stream::Stream(asio::io_service& io_service)
{
	writer = std::make_shared<UdpAsyncWriter>(io_service);
}

void mtt::utp::Stream::connect(const udp::endpoint& e, uint16_t bindPort)
{
	state.step = StateType::SYN_SEND;
	state.id_recv = (uint16_t)rand();
	state.id_send = state.id_recv + 1;

	//state.sequence = (uint16_t)rand();

	writer->setAddress(e);
	writer->setBindPort(bindPort);

	sendSyn();
	connection.lastReceive = TimeClock::now();
	connection.nextTimeout = connection.lastReceive + std::chrono::milliseconds(packetTimeout());
}

void mtt::utp::Stream::connect(const udp::endpoint& e, uint16_t bindPort, const MessageHeader& header)
{
	state.step = StateType::SYN_RECV;
	state.id_send = header.connection_id;
	state.id_recv = state.id_send + 1;
	
	state.sequence = (uint16_t)rand();
	state.ack = header.seq_nr;

	writer->setAddress(e);
	writer->setBindPort(bindPort);

	sendState();
	connection.lastReceive = TimeClock::now();
	connection.nextTimeout = connection.lastReceive + std::chrono::milliseconds(packetTimeout());
}

void mtt::utp::Stream::write(const DataBuffer& data)
{
	std::lock_guard<std::mutex> guard(state.mutex);

	if (state.step == StateType::CLOSED)
		return;

	uint32_t dataPosition = 0;
	while (dataPosition < data.size())
	{
		uint32_t dataSizeLeft = uint32_t(data.size()) - dataPosition;

		if (connection.wnd_size < sending.sentDataWithoutAckSize || !sending.prepared.buffer.empty())
		{
			size_t currentPreparedSize = sending.prepared.buffer.size();
			if (dataSizeLeft + currentPreparedSize > sending.prepared.buffer.capacity())
			{
				std::swap(sending.prepared.buffer, sending.prepared.buffer2);
				currentPreparedSize -= sending.prepared.bufferPos;
				sending.prepared.buffer.resize(currentPreparedSize + dataSizeLeft);
				memcpy(sending.prepared.buffer.data(), sending.prepared.buffer2.data() + sending.prepared.bufferPos, currentPreparedSize);
				sending.prepared.bufferPos = 0;
			}
			else
				sending.prepared.buffer.resize(currentPreparedSize + dataSizeLeft);

			memcpy(sending.prepared.buffer.data() + currentPreparedSize, data.data() + dataPosition, dataSizeLeft);
			UTP_LOG("Prepared " << dataSizeLeft << " bytes for later sending, totally prepared size " << sending.prepared.buffer.size());

			break;
		}

		dataPosition += sendData(data.data() + dataPosition, dataSizeLeft);
	}
}

bool mtt::utp::Stream::readUdpPacket(const MessageHeader& header, const BufferView& data)
{
	switch (header.getType())
	{
	case MessageType::ST_STATE:
		handleStateMessage(header);
		break;
	case MessageType::ST_DATA:
		handleDataMessage(header, data);
		break;
	case MessageType::ST_FIN:
		handleFinMessage(header);
		break;
	case MessageType::ST_RESET:
		handleResetMessage(header);
		break;
	default:
		return false;
	}

	return true;
}

void mtt::utp::Stream::readFinish()
{
	if (receiving.state.received)
	{
		std::lock_guard<std::mutex> guard(state.mutex);
		sendState();
	}
	if (receiving.state.appended)
	{
		if (onReceiveCallback)
		{
			size_t consumed = onReceiveCallback({ receiving.receiveBuffer });

			if (consumed < receiving.receiveBuffer.size())
			{
				auto tmp = DataBuffer(receiving.receiveBuffer.begin() + consumed, receiving.receiveBuffer.end());
				receiving.receiveBuffer.resize(tmp.size());
				memcpy(receiving.receiveBuffer.data(), tmp.data(), tmp.size());
			}
			else
				receiving.receiveBuffer.clear();
		}
	}

	receiving.state.received = false;
	receiving.state.appended = false;
}

bool mtt::utp::Stream::refresh(TimePoint now)
{
	std::lock_guard<std::mutex> guard(state.mutex);

	if (connection.nextTimeout < now)
	{
		if (connection.lastReceive + std::chrono::milliseconds(MaxStreamTimeout) < now || state.step >= StateType::CLOSING)
			close();
		else if (sending.sentDataWithoutAckSize > 0)
		{
			for (auto& s : sending.sentData)
				s.needResend = true;

			UTP_LOG("Timeout detected, sent data marked for resend: " << sending.sentData.size());
			if (!sending.sentData.empty())
				resendPackets(sending.sentData.front().sequence);
		}
		else if (state.step == StateType::SYN_SEND)
		{
			if (connection.timeoutCount > 0)
				close();
			else
				sendSyn();
		}
		else if (state.step == StateType::SYN_RECV)
		{
			if (connection.timeoutCount > 0)
				close();
			else
				sendState();
		}

		connection.timeoutCount++;
		connection.nextTimeout = now + std::chrono::milliseconds(packetTimeout());
	}

	return state.step != StateType::CLOSED;
}

uint16_t mtt::utp::Stream::getId()
{
	return state.id_recv;
}

const udp::endpoint& mtt::utp::Stream::getEndpoint()
{
	return writer->getEndpoint();
}

void mtt::utp::Stream::close()
{
	if (state.step != StateType::CLOSED)
	{
		state.step = StateType::CLOSED;

		if (onCloseCallback)
			onCloseCallback(0);
	}
}

void mtt::utp::Stream::sendSyn()
{
	sending.stateBuffer.resize(20);

	utp::MessageHeader& header = (utp::MessageHeader&) * sending.stateBuffer.data();
	header.setType(ST_SYN);
	header.seq_nr = state.sequence++;
	header.ack_nr = state.ack;
	header.connection_id = state.id_recv;

	header.wnd_size = connection.wnd_size;
	header.timestamp_microseconds = timestampMicro();
	header.timestamp_difference_microseconds = 0;

	UTP_LOG("Send ST_SYN, conn id: " << state.id_send);
	writer->write(sending.stateBuffer);
}

void mtt::utp::Stream::sendState()
{
	prepareStateHeader(ST_STATE);
	writer->write(sending.stateBuffer);
	UTP_LOG("Send ST_STATE, seq " << state.sequence << ",  ack " << state.ack);
}

void mtt::utp::Stream::sendFin()
{
	prepareStateHeader(ST_FIN);
	((utp::MessageHeader*) sending.stateBuffer.data())->ack_nr = connection.eof_ack;
	writer->write(sending.stateBuffer);
	UTP_LOG("Send ST_FIN, seq " << state.sequence << ",  ack " << connection.eof_ack);
}

void mtt::utp::Stream::prepareStateHeader(MessageType type)
{
	const size_t headerSize = sizeof(utp::MessageHeader);

	sending.stateBuffer.resize(headerSize);
	memset(sending.stateBuffer.data(), 0, headerSize);

	utp::MessageHeader& header = (utp::MessageHeader&) * sending.stateBuffer.data();
	header.setType(type);
	header.seq_nr = state.sequence;
	header.ack_nr = state.ack;
	header.connection_id = state.id_send;

	header.wnd_size = connection.wnd_size;
	header.timestamp_microseconds = timestampMicro();
	header.timestamp_difference_microseconds = connection.last_reply_delay;
}

void mtt::utp::Stream::flushSendData()
{
	if (sending.duplicateAckCounter > MaxDuplicateAckBeforeResend)
		resendPackets(connection.last_ack + 1);
	else
	{
		uint16_t resendCount = 0;
		for (auto& s : sending.sentData)
		{
			if(!s.needResend)
				break;
			resendCount++;
		}

		if (resendCount)
		{
			resendCount = std::min(resendCount, uint16_t(100));
			UTP_LOG("Resending " << resendCount << " packets");
			resendPackets(connection.last_ack + resendCount);
		}
	}

	uint32_t preparedBufferPos = sending.prepared.bufferPos;
	while (preparedBufferPos < sending.prepared.buffer.size() && connection.wnd_size > sending.sentDataWithoutAckSize)
	{
		preparedBufferPos += sendData(sending.prepared.buffer.data() + preparedBufferPos, uint32_t(sending.prepared.buffer.size()) - preparedBufferPos);
	}

	if (preparedBufferPos != sending.prepared.bufferPos)
	{
		UTP_LOG("Flushed " << preparedBufferPos - sending.prepared.bufferPos << " bytes, remaining unsent data size " << sending.prepared.buffer.size() - preparedBufferPos);

		if (preparedBufferPos >= sending.prepared.buffer.size())
		{
			sending.prepared.buffer.clear();
			sending.prepared.bufferPos = 0;
		}
		else
		{
			constexpr uint32_t SendBufferLeftoverLimit = 100000;	//we dont want to move data in buffer after each packet
			if (preparedBufferPos > SendBufferLeftoverLimit)
			{
				size_t dataLeft = sending.prepared.buffer.size() - preparedBufferPos;
				std::swap(sending.prepared.buffer, sending.prepared.buffer2);
				sending.prepared.buffer.resize(dataLeft);
				memcpy(sending.prepared.buffer.data(), sending.prepared.buffer2.data() + preparedBufferPos, dataLeft);

				sending.prepared.bufferPos = 0;
			}
			else
				sending.prepared.bufferPos = preparedBufferPos;
		}
	}
}

bool mtt::utp::Stream::updateState(const MessageHeader& header)
{
	UTP_LOG("updateState, my seq ack " << state.ack << "/ header seq " << header.seq_nr << ", my seq " << state.sequence << "/ header seq ack " << header.ack_nr << ", latest ack " << connection.last_ack);

	if (header.getType() == ST_DATA && idLowerThan(header.seq_nr, state.ack))	//too old, we got newer already
	{
		UTP_LOG("packet old, dropped");
		return false;
	}

	bool connected = false;
	{
		std::lock_guard<std::mutex> guard(state.mutex);

		ackSentDataBuffer(header);

		connection.last_reply_delay = header.timestamp_microseconds - timestampMicro();
		connection.peer_wnd_size = header.wnd_size;

		if (state.step < StateType::CONNECTED)
		{
			if (state.step == StateType::SYN_SEND)
				state.ack = header.seq_nr - 1;

			state.step = StateType::CONNECTED;
			connection.last_ack = header.ack_nr;
			connected = true;
		}

		if (header.extension)
		{
			uint8_t extType = header.extension;
			auto ext = (const ExtensionMessage*)(&header + 1);
			while (extType)
			{
				handleExtension(header, *ext, extType);
				extType = ext->nextExtensionType;
				ext = (const ExtensionMessage*)((const char*)(ext + 1) + ext->len);
			}
		}

		flushSendData();

		connection.lastReceive = TimeClock::now();
		connection.nextTimeout = connection.lastReceive + std::chrono::milliseconds(packetTimeout());
	}

	if (connected && onConnectCallback)
	{
		UTP_LOG("Connected, seq " << header.seq_nr << ", ack " << header.ack_nr);

		if (onConnectCallback)
			onConnectCallback(true);

		onConnectCallback = nullptr;
	}

	return true;
}

void mtt::utp::Stream::handleStateMessage(const MessageHeader& header)
{
	updateState(header);
}

void mtt::utp::Stream::handleDataMessage(const MessageHeader& header, const BufferView& data)
{
	if (!data.size || !updateState(header) || state.step == StateType::CLOSED)
		return;

	if (header.seq_nr == state.ack + 1)
	{
		appendReceiveBuffer(data);
		state.ack++;
		flushReceivedData();
		receiving.state.appended = true;
	}
	else
	{
		if (idHigherThan(header.seq_nr, state.ack))
			storeReceivedData(header.seq_nr, data);
		return;
	}

	receiving.state.received = true;

	if (state.step == StateType::CLOSING)
		checkEof();
}

void mtt::utp::Stream::handleFinMessage(const MessageHeader& msg)
{
	std::lock_guard<std::mutex> guard(state.mutex);

	if (state.step != StateType::CLOSED)
	{
		connection.eof_ack = msg.seq_nr;
		state.step = StateType::CLOSING;
		clearSendData();
	}

	UTP_LOG("Received FIN with eof " << msg.seq_nr);

	checkEof();
}

void mtt::utp::Stream::handleResetMessage(const MessageHeader&)
{
	std::lock_guard<std::mutex> guard(state.mutex);

	UTP_LOG("Received RESET");
	clearSendData();

	close();
}

void mtt::utp::Stream::handleExtension(const MessageHeader& msg, const ExtensionMessage& ext, uint8_t type)
{
	UTP_LOG("handleExtension type " << (int)type);

	if (type == ExtensionMessage::SelectiveAcksExtensionType)
	{
		ackSentDataBuffer(msg, (const uint8_t*)(&ext + 1), ext.len);
	}
}

std::pair<DataBuffer&, uint16_t> mtt::utp::Stream::createDataBuffer(size_t size)
{
	for (uint16_t i = 0; i < dataBuffers.size(); i++)
	{
		if (dataBuffers[i].empty())
		{
			dataBuffers[i].resize(size);
			return { dataBuffers[i], i };
		}
	}

	dataBuffers.push_back(DataBuffer(size));

	return { dataBuffers.back(), uint16_t(dataBuffers.size() - 1) };
}

uint32_t mtt::utp::Stream::packetTimeout() const
{
	if (state.step < StateType::CONNECTED)
		return 3000;

	uint32_t timeout = (int)std::max(500, m_rtt.mean() + m_rtt.avg_deviation() * 2);

	if (connection.timeoutCount > 0)
		timeout += (1 << (connection.timeoutCount - 1)) * 1000;

	if (timeout > MaxStreamTimeout)
		timeout = MaxStreamTimeout;

	return timeout;
}

uint32_t mtt::utp::Stream::sendData(const uint8_t* data, uint32_t dataSize)
{
	if (dataSize > sending.mtu.workingMaxSize)
	{
		if (!sending.mtu.probeSendingFinished && sending.mtu.probeId == MTU_PROBE_IDLE_ID && state.sequence != MTU_PROBE_IDLE_ID)
		{
			dataSize = std::min(dataSize, (sending.mtu.workingMaxSize + sending.mtu.Max) / 2);
			sending.mtu.probeId = state.sequence;
			UTP_LOG("Packet seq " << state.sequence << " selected as mtu probe with size " << dataSize);
		}
		else
			dataSize = sending.mtu.workingMaxSize;
	}

	auto [buffer, bufferIdx] = createDataBuffer(dataSize + sizeof(utp::MessageHeader));

	utp::MessageHeader& header = (utp::MessageHeader&) * buffer.data();
	memset(buffer.data(), 0, 20);
	header.setType(ST_DATA);
	header.connection_id = state.id_send;
	header.seq_nr = state.sequence++;
	header.ack_nr = state.ack;
	header.wnd_size = connection.wnd_size;

	auto now = TimeClock::now();
	header.timestamp_microseconds = timestampMicro(now);
	header.timestamp_difference_microseconds = connection.last_reply_delay;

	UTP_LOG("Send ST_DATA, seq " << header.seq_nr << ",  ack " << header.ack_nr << ", data size " << dataSize);

	memcpy(buffer.data() + sizeof(utp::MessageHeader), data, dataSize);

	sending.sentData.push_back({ now, bufferIdx, header.seq_nr });
	sending.sentDataWithoutAckSize += dataBuffers[bufferIdx].size();

	auto opt = header.seq_nr == sending.mtu.probeId ? UdpAsyncWriter::WriteOption::DontFragment : UdpAsyncWriter::WriteOption::None;
	writer->write(BufferView{ dataBuffers[bufferIdx] }, opt);

	return dataSize;
}

void mtt::utp::Stream::ackSentDataBuffer(const MessageHeader& header)
{
	size_t acked = 0;
	for (auto sent : sending.sentData)
	{
		if(idHigherThan(sent.sequence, header.ack_nr))
			break;

		ackSentPacket(header, sent);
		acked++;
	}

	if (acked)
	{
		sending.sentData.erase(sending.sentData.begin(), sending.sentData.begin() + acked);
		sending.duplicateAckCounter = 0;
		connection.last_ack = header.ack_nr;

		UTP_LOG("Received Ack " << header.ack_nr << ", totaly acked " << acked << " packets, remaining unacked " << sending.sentData.size());
	}
	else if (header.getType() != ST_DATA && connection.last_ack == header.ack_nr)
	{
		sending.duplicateAckCounter++;
	}
}

void mtt::utp::Stream::ackSentDataBuffer(const MessageHeader& header, const uint8_t* selectiveAcks, uint8_t len)
{
	uint32_t ackedAfterMissing = 0;
	uint16_t lastAcks[MaxDuplicateAckBeforeResend + 1];	//remember last Max acked packets

	for (int i = 0; i < len; i++)
	{
		uint8_t ackMask = 0b1;
		for (int j = 0; j < 8; j++)
		{
			if (selectiveAcks[i] & ackMask)
			{
				uint16_t ack = header.ack_nr + uint16_t(2 + i * 8 + j);
				lastAcks[ackedAfterMissing++ % std::size(lastAcks)] = ack;

				for (auto it = sending.sentData.begin(); it != sending.sentData.end(); it++)
				{
					if (!idLowerThan(it->sequence, ack))
					{
						if (it->sequence == ack)
						{
							ackSentPacket(header, *it);
							sending.sentData.erase(it);

							UTP_LOG("Selective ack idx " << ack);
						}

						break;
					}
				}
			}
			ackMask <<= 1;
		}
	}

	UTP_LOG("Selective acks with " << len << " bytes (" << hexToString(selectiveAcks, len) << "), missing start " << connection.last_ack + 1 << ", acked after missing " << ackedAfterMissing << ", remaining data without ack " << sending.sentData.size());

	if (ackedAfterMissing > MaxDuplicateAckBeforeResend)	 //if Max+ acks received, resend all unacked packets before Max acks
	{
		uint16_t lastResend = lastAcks[ackedAfterMissing % std::size(lastAcks)] - 1;
		UTP_LOG("Selective acks resending packets from " << connection.last_ack + 1 << " to " << lastResend);
		resendPackets(lastResend);
	}
}

void mtt::utp::Stream::ackSentPacket(const MessageHeader& header, const Sending::SentDataPacket& packet)
{
	if (packet.sequence == sending.mtu.probeId && packet.sequence != MTU_PROBE_IDLE_ID)
	{
		sending.mtu.workingMaxSize = uint32_t(dataBuffers[packet.bufferIdx].size()) - sizeof(utp::MessageHeader);
		UTP_LOG("Successful mtu probe " << packet.sequence << " increased working packet size to " << sending.mtu.workingMaxSize);
		sending.mtu.probeId = MTU_PROBE_IDLE_ID;

		if (sending.mtu.workingMaxSize > 1400)
			sending.mtu.probeSendingFinished = true;
	}

	if (!packet.resent)
	{
		auto receiveTime = TimeClock::now();
		if (receiveTime > packet.sendTime)
		{
			auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(receiveTime - packet.sendTime).count();
			m_rtt.add_sample(int(rtt));
		}
	}

	sending.sentDataWithoutAckSize -= dataBuffers[packet.bufferIdx].size();
	dataBuffers[packet.bufferIdx].clear();
}

void mtt::utp::Stream::resendPackets(uint16_t lastIdx)
{
	for (auto& packet : sending.sentData)
	{
		if(idHigherThan(packet.sequence, lastIdx))
			break;

		if(packet.resent && !packet.needResend)
			continue;

		packet.resent = true;
		packet.needResend = false;
		writer->write(BufferView{ dataBuffers[packet.bufferIdx] });
		UTP_LOG("Resend packet " << packet.sequence);
	}

	sending.duplicateAckCounter = 0;
}

void mtt::utp::Stream::clearSendData()
{
	sending.prepared.buffer.clear();
	sending.prepared.bufferPos = 0;

	sending.sentDataWithoutAckSize = 0;
	for (auto& b : sending.sentData)
	{
		dataBuffers[b.bufferIdx].clear();
	}
	sending.sentData.clear();
}

void mtt::utp::Stream::checkEof()
{
	if (state.ack + 1 == connection.eof_ack)
	{
		sendFin();
		close();
	}
}

void mtt::utp::Stream::flushReceivedData()
{
	if (receiving.receivedData.empty())
		return;

	std::lock_guard<std::mutex> guard(state.mutex);

	int flushed = 0;
	for (auto& d : receiving.receivedData)
	{
		if (d.sequence == state.ack + 1)
		{
			auto& buffer = dataBuffers[d.bufferIdx];
			appendReceiveBuffer(buffer);
			state.ack++;
			flushed++;
		}
		else
			break;
	}

	if (flushed)
	{
		receiving.receivedData.erase(receiving.receivedData.begin(), receiving.receivedData.begin() + flushed);
		UTP_LOG("Flushed " << flushed << " received blocks, to current ack: " << state.ack << ", remaining unflushed count: " << receiving.receivedData.size());
	}
}

void mtt::utp::Stream::appendReceiveBuffer(const BufferView& data)
{
	size_t currentSize = receiving.receiveBuffer.size();
	if (currentSize && (receiving.receiveBuffer.capacity() < (currentSize + data.size)))
	{
		auto tmp = std::move(receiving.receiveBuffer);
		receiving.receiveBuffer.resize(currentSize + data.size);
		memcpy(receiving.receiveBuffer.data(), tmp.data(), currentSize);
	}
	else
		receiving.receiveBuffer.resize(currentSize + data.size);

	memcpy(receiving.receiveBuffer.data() + currentSize, data.data, data.size);
}

void mtt::utp::Stream::storeReceivedData(uint16_t sequenceId, const BufferView& data)
{
	std::lock_guard<std::mutex> guard(state.mutex);

	auto [buffer, bufferIdx] = createDataBuffer(data.size);
	memcpy(buffer.data(), data.data, data.size);

	//ordered insert
	size_t idx = 0;
	for (; idx < receiving.receivedData.size(); idx++)
	{
		if (receiving.receivedData[idx].sequence == sequenceId)
			return;
		if(idHigherThan(receiving.receivedData[idx].sequence, sequenceId))
			break;
	}
	receiving.receivedData.insert(receiving.receivedData.begin() + idx, { bufferIdx, sequenceId });

	UTP_LOG("Store data idx " << sequenceId << ", current ack: " << state.ack << ", current stored data count: " << receiving.receivedData.size());
}

uint32_t mtt::utp::parseHeaderSize(const MessageHeader* header, size_t dataSize)
{
	uint8_t extensionType = header->extension;
	auto ext = (ExtensionMessage*)(header + 1);
	size_t dataLeft = dataSize - 20;

	while (extensionType)
	{
		if (dataLeft < sizeof(ExtensionMessage))
			return 0;

		dataLeft -= sizeof(ExtensionMessage);

		if(dataLeft < ext->len)
			return 0;

		dataLeft -= ext->len;
		extensionType = ext->nextExtensionType;
		ext = (ExtensionMessage*)((const char*)(ext + 1) + ext->len);
	}

	return uint32_t(dataSize - dataLeft);
}

uint32_t mtt::utp::timestampMicro(const TimePoint& now)
{
	return std::uint32_t((std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()) & 0xffffffff);
}

uint32_t mtt::utp::timestampMicro()
{
	return timestampMicro(TimeClock::now());
}

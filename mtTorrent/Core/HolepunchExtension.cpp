#include "HolepunchExtension.h"
#include "utils/PacketHelper.h"
#include "PeerMessage.h"
#include "PeerCommunication.h"

using namespace mtt;
using namespace mtt::ext;

bool UtHolepunch::Load(const BufferView& buffer, Message& msg)
{
	if (buffer.size < 8)
		return false;

	msg.id = (MessageId)buffer.data[0];

	if (buffer.data[1] > 1)
		return false;

	bool ipv6 = buffer.data[1] == 1;
	if (ipv6 && buffer.size < 20)
		return false;

	int addrSize = msg.addr.parse(buffer.data + 2, ipv6);

	if (msg.id == UtHolepunch::Error)
		msg.e = (ErrorCode) PacketReader(buffer.data + 2 + addrSize, 4).pop32();

	return true;
}

void UtHolepunch::sendConnect(const Addr& addr)
{
	send(addr, UtHolepunch::Connect, E_None);
}

void UtHolepunch::sendRendezvous(const Addr& addr)
{
	send(addr, UtHolepunch::Rendezvous, E_None);
}

void UtHolepunch::sendError(const Addr& addr, ErrorCode e)
{
	send(addr, UtHolepunch::Error, e);
}

void UtHolepunch::send(const Addr& addr, MessageId id, ErrorCode e)
{
	DataBuffer data;
	data.reserve(32);

	data.resize(sizeof(uint32_t)); //length
	data.push_back(mtt::PeerMessage::Extended);
	data.push_back(messageId);

	data.push_back(id);
	data.push_back(addr.ipv6);
	auto addrData = addr.toData();
	data.insert(data.end(), addrData.begin(), addrData.end());

	if (id == UtHolepunch::Error)
		PacketBuilder::Append32(data, e);

	PacketBuilder::Assign32(data.data(), (uint32_t)(data.size() - sizeof(uint32_t)));

	peer->send(data);
}

#include "Network.h"
#include "ByteSwap.h"

Addr::Addr()
{
	memset(addrBytes, 0, 16);
}

Addr::Addr(const DataBuffer& ip, uint16_t port)
{
	set(ip, port);
}

Addr::Addr(uint32_t ip, uint16_t port)
{
	set(ip, port);
}

Addr::Addr(const uint8_t* ip, uint16_t port, bool isIpv6)
{
	set(ip, port, isIpv6);
}

Addr::Addr(const uint8_t* buffer, bool v6)
{
	parse(buffer, v6);
}

Addr::Addr(const asio::ip::address& addr, uint16_t port_num)
{
	set(addr, port_num);
}

Addr::Addr(const char* str)
{
	auto a = asio::ip::address::from_string(str);

	size_t portStart = strlen(str);
	while (portStart > 0)
		if (str[portStart] == ':')
			break;
		else
			portStart--;

	set(a, (uint16_t)strtoul(str + portStart + 1, 0, 10));
}

static Addr emptyAddr;

const Addr& Addr::Empty()
{
	return emptyAddr;
}

void Addr::set(const uint8_t* ip, uint16_t p, bool isIpv6)
{
	memcpy(addrBytes, ip, isIpv6 ? 16 : 4);
	port = p;
	ipv6 = isIpv6;
}

void Addr::set(uint32_t ip, uint16_t p)
{
	memcpy(addrBytes, &ip, 4);
	port = p;
	ipv6 = false;
}

void Addr::set(const DataBuffer& ip, uint16_t p)
{
	memcpy(addrBytes, ip.data(), ip.size());
	port = p;
	ipv6 = ip.size() > 4;
}

void Addr::set(const asio::ip::address& addr, uint16_t port_num)
{
	port = port_num;
	ipv6 = addr.is_v6();

	if (ipv6)
	{
		auto data = addr.to_v6().to_bytes();
		memcpy(addrBytes, data.data(), data.size());
	}
	else
	{
		auto data = addr.to_v4().to_bytes();
		memcpy(addrBytes, data.data(), data.size());
	}
}

int Addr::parse(const uint8_t* buffer, bool v6)
{
	ipv6 = v6;
	int addrSize = v6 ? 16 : 4;
	memcpy(addrBytes, buffer, addrSize);
	port = swap16(*reinterpret_cast<const uint16_t*>(buffer + addrSize));

	return 2 + addrSize;
}

asio::ip::udp::endpoint Addr::toUdpEndpoint() const
{
	return ipv6 ?
		udp::endpoint(asio::ip::address_v6(*reinterpret_cast<const asio::ip::address_v6::bytes_type*>(addrBytes)), port) :
		udp::endpoint(asio::ip::address_v4(*reinterpret_cast<const asio::ip::address_v4::bytes_type*>(addrBytes)), port);
}

asio::ip::tcp::endpoint Addr::toTcpEndpoint() const
{
	return ipv6 ?
		tcp::endpoint(asio::ip::address_v6(*reinterpret_cast<const asio::ip::address_v6::bytes_type*>(addrBytes)), port) :
		tcp::endpoint(asio::ip::address_v4(*reinterpret_cast<const asio::ip::address_v4::bytes_type*>(addrBytes)), port);
}

std::string Addr::toString() const
{
	return toUdpEndpoint().address().to_string() + ":" + std::to_string(port);
}

uint32_t Addr::toUint()
{
	return *reinterpret_cast<uint32_t*>(addrBytes);
}

bool Addr::operator==(const Addr& r)
{
	if (r.ipv6 && !ipv6)
		return false;

	return memcmp(addrBytes, r.addrBytes, ipv6 ? 16 : 4) == 0;
}

BufferView::BufferView()
{
}

BufferView::BufferView(const uint8_t* d, size_t s)
{
	data = d;
	size = s;
}

BufferView::BufferView(const DataBuffer& d)
{
	data = d.data();
	size = d.size();
}

void BufferView::store(const uint8_t* d, size_t s)
{
	localbuffer.resize(s);
	memcpy(localbuffer.data(), d, s);
	data = localbuffer.data();
	size = localbuffer.size();
}

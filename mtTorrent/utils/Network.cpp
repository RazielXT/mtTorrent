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

Addr Addr::fromString(const char* str)
{
	auto a = asio::ip::address::from_string(str);

	size_t portStart = strlen(str);
	while (portStart > 0)
		if (str[portStart] == ':')
			break;
		else
			portStart--;

	return Addr(a, (uint16_t)strtoul(str + portStart + 1, nullptr, 10));
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
	if (addr.is_unspecified())
		return;

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

static void iToString(int i, std::string& s)
{
	if (i > 99)
	{
		int value = (i / 100);
		s += '0' + value;
		i -= value * 100;
	}
	if (i > 9)
	{
		int value = (i / 10);
		s += '0' + value;
		i -= value * 10;
	}
	s += '0' + i;
}

static std::string toIpv4String(const uint8_t* buffer, uint16_t port)
{
	std::string out;
	out.reserve(21);
	iToString(buffer[0], out);
	out += '.';
	iToString(buffer[1], out);
	out += '.';
	iToString(buffer[2], out);
	out += '.';
	iToString(buffer[3], out);
	out += ':';

	char buff[5] = {};
	char* ptr = &buff[5];
	do
	{
		*--ptr = '0' + port % 10;
		port /= 10;
	}
	while (port != 0);
	out.append(ptr, &buff[5]);

	return out;
}

std::string Addr::toString() const
{
	return ipv6 ? toUdpEndpoint().address().to_string() : toIpv4String(addrBytes, port);
}

uint32_t Addr::toUint() const
{
	return *(uint32_t*)(addrBytes);
}

std::string Addr::toData() const
{
	std::string data;
	int addrSize = ipv6 ? 16 : 4;
	data.resize(addrSize + 2);
	memcpy(data.data(), addrBytes, addrSize);
	*reinterpret_cast<uint16_t*>(data.data() + addrSize) = swap16(port);

	return data;
}

bool Addr::valid() const
{
	return port != 0;
}

bool Addr::operator<(const Addr& r) const
{
	return port < r.port || (port == r.port && memcmp(addrBytes, r.addrBytes, ipv6 ? 16 : 4) < 0);
}

bool Addr::operator==(const Addr& r) const
{
	return port == r.port && memcmp(addrBytes, r.addrBytes, ipv6 ? 16 : 4) == 0;
}

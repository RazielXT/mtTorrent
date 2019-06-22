#pragma once

#define _WIN32_WINDOWS 0x0603
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <asio.hpp>

using asio::ip::tcp;
using asio::ip::udp;

using DataBuffer = std::vector<uint8_t>;

struct Addr
{
	Addr();
	Addr(uint8_t* buffer, bool v6);
	Addr(uint8_t* ip, uint16_t port, bool isIpv6);
	Addr(uint32_t ip, uint16_t port);
	Addr(DataBuffer ip, uint16_t port);
	Addr(const asio::ip::address& addr, uint16_t port_num);
	Addr(const char* str);

	uint8_t addrBytes[16];
	uint16_t port = 0;
	bool ipv6;

	void set(uint8_t* ip, uint16_t port, bool isIpv6);
	void set(DataBuffer ip, uint16_t port);
	void set(uint32_t ip, uint16_t port);
	void set(const asio::ip::address& addr, uint16_t port_num);

	int parse(uint8_t* buffer, bool v6);
	asio::ip::udp::endpoint toUdpEndpoint() const;
	std::string toString() const;
	uint32_t toUint();

	bool operator==(const Addr& r);
};

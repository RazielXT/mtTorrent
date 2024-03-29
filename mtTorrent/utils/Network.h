#pragma once

#include "DataBuffer.h"

#include <asio.hpp>

#ifdef MTT_WITH_SSL
#include <asio/ssl.hpp>
#endif

using asio::ip::tcp;
using asio::ip::udp;

struct Addr
{
	Addr();
	Addr(const uint8_t* buffer, bool v6);
	Addr(const uint8_t* ip, uint16_t port, bool isIpv6);
	Addr(uint32_t ip, uint16_t port);
	Addr(const DataBuffer& ip, uint16_t port);
	Addr(const asio::ip::address& addr, uint16_t port_num);

	static Addr fromString(const char* str);
	static Addr fromString(const char* str, uint16_t port);
	static asio::ip::address asioFromString(const char* str);

	uint8_t addrBytes[16];
	uint16_t port = 0;
	bool ipv6 = false;

	void set(const uint8_t* ip, uint16_t port, bool isIpv6);
	void set(const DataBuffer& ip, uint16_t port);
	void set(uint32_t ip, uint16_t port);
	void set(const asio::ip::address& addr, uint16_t port_num);

	int parse(const uint8_t* buffer, bool v6);
	asio::ip::udp::endpoint toUdpEndpoint() const;
	asio::ip::tcp::endpoint toTcpEndpoint() const;
	std::string toString() const;
	uint32_t toUint() const;
	std::string toData() const;

	bool valid() const;

	bool operator==(const Addr& r) const;
	bool operator<(const Addr& r) const;
};

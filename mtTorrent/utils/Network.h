#pragma once

#define _WIN32_WINDOWS 0x0603
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <asio.hpp>
#ifdef MTT_WITH_SSL
#include <asio/ssl.hpp>
#endif
using asio::ip::tcp;
using asio::ip::udp;

using DataBuffer = std::vector<uint8_t>;

struct BufferView
{
	BufferView();
	BufferView(const DataBuffer&);
	BufferView(DataBuffer&&);
	BufferView(const uint8_t* data, size_t size);

	const uint8_t* data = nullptr;
	size_t size = 0;

	void store(const uint8_t* data, size_t size);
	void store();
private:
	DataBuffer localbuffer;
};

struct Addr
{
	Addr();
	Addr(const uint8_t* buffer, bool v6);
	Addr(const uint8_t* ip, uint16_t port, bool isIpv6);
	Addr(uint32_t ip, uint16_t port);
	Addr(const DataBuffer& ip, uint16_t port);
	Addr(const asio::ip::address& addr, uint16_t port_num);

	static Addr fromString(const char* str);

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
	uint32_t toUint();

	bool operator==(const Addr& r);
};

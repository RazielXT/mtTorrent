#pragma once

#define _WIN32_WINDOWS 0x0603
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

using DataBuffer = std::vector<uint8_t>;

struct Addr
{
	Addr();
	Addr(uint8_t* buffer, bool v6);
	Addr(uint8_t* ip, uint16_t port, bool isIpv6);
	Addr(uint32_t ip, uint16_t port);
	Addr(DataBuffer ip, uint16_t port);

	uint8_t addrBytes[16];
	uint16_t port;
	bool ipv6;

	void set(uint8_t* ip, uint16_t port, bool isIpv6);
	void set(DataBuffer ip, uint16_t port);
	void set(uint32_t ip, uint16_t port);
	void set(const boost::asio::ip::address& addr, uint16_t port_num);

	size_t parse(uint8_t* buffer, bool v6);
	bool operator==(const Addr& r);
};

void openSslSocket(ssl_socket& sock, tcp::resolver& resolver, const char* hostname);
std::string sendHttpsRequest(ssl_socket& socket, tcp::resolver& resolver, boost::asio::streambuf& request, const char* hostname);

void openTcpSocket(tcp::socket& sock, tcp::resolver& resolver, const char* hostname, const char* port);
DataBuffer sendTcpRequest(tcp::socket& socket, DataBuffer& request);

DataBuffer sendUdpRequest(udp::socket& socket, udp::resolver& resolver, DataBuffer& request, const char* hostname, const char* port = "80", int32_t timeout = 5000, bool ipv6 = false);
DataBuffer sendUdpRequest(udp::socket& socket, DataBuffer& request, const char* ip, uint16_t port, int32_t timeout = 5000);
DataBuffer sendUdpRequest(udp::socket& socket, DataBuffer& request, uint8_t* ip, uint16_t port, int32_t timeout = 5000, bool ipv6 = false);
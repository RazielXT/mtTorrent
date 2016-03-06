#pragma once

#define _WIN32_WINDOWS 0x0603
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

void openSslSocket(ssl_socket& sock, tcp::resolver& resolver, const char* server);
std::string sendHttpsRequest(ssl_socket& socket, tcp::resolver& resolver, boost::asio::streambuf& request, const char* server);

void openTcpSocket(tcp::socket& sock, tcp::resolver& resolver, const char* server, const char* port);
std::vector<char> sendTcpRequest(tcp::socket& socket, std::vector<char>& request);

std::vector<char> sendUdpRequest(udp::socket& socket, udp::resolver& resolver, std::vector<char> request, const char* server, const char* port = "80");

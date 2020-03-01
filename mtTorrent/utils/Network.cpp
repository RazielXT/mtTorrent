#include "utils\Network.h"

Addr::Addr()
{
	memset(addrBytes, 0, 16);
}

Addr::Addr(DataBuffer ip, uint16_t port)
{
	set(ip, port);
}

Addr::Addr(uint32_t ip, uint16_t port)
{
	set(ip, port);
}

Addr::Addr(uint8_t* ip, uint16_t port, bool isIpv6)
{
	set(ip, port, isIpv6);
}

Addr::Addr(uint8_t* buffer, bool v6)
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

void Addr::set(uint8_t* ip, uint16_t p, bool isIpv6)
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

void Addr::set(DataBuffer ip, uint16_t p)
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

int Addr::parse(uint8_t* buffer, bool v6)
{
	ipv6 = v6;
	int addrSize = v6 ? 16 : 4;
	memcpy(addrBytes, buffer, addrSize);
	port = _byteswap_ushort(*reinterpret_cast<const uint16_t*>(buffer + addrSize));

	return 2 + addrSize;
}

asio::ip::udp::endpoint Addr::toUdpEndpoint() const
{
	return ipv6 ?
		udp::endpoint(asio::ip::address_v6(*reinterpret_cast<const asio::ip::address_v6::bytes_type*>(addrBytes)), port) :
		udp::endpoint(asio::ip::address_v4(*reinterpret_cast<const asio::ip::address_v4::bytes_type*>(addrBytes)), port);
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
#ifdef MTT_WITH_SSL
void openSslSocket(ssl_socket& sock, tcp::resolver& resolver, const char* hostname)
{
	SSL_set_tlsext_host_name(sock.native_handle(), hostname);

	tcp::resolver::query query(hostname, "https");
	asio::connect(sock.lowest_layer(), resolver.resolve(query));
	sock.lowest_layer().set_option(tcp::no_delay(true));

	// Perform SSL handshake and verify the remote host's
	// certificate.
	//sock.set_verify_mode(ssl::verify_peer);
	//sock.set_verify_callback(ssl::rfc2818_verification(server));

	sock.set_verify_mode(asio::ssl::verify_none);
	sock.handshake(ssl_socket::client);
}

std::vector<std::string> outFHistory;
std::vector<std::string> outHistory;
std::vector<std::string> history;

std::string sendHttpsRequest(ssl_socket& socket, asio::streambuf& request)
{
	// Send the request.
	asio::write(socket, request);

	// Read the response status line. The response streambuf will automatically
	// grow to accommodate the entire line. The growth may be limited by passing
	// a maximum size to the streambuf constructor.
	asio::streambuf response;
	//asio::error_code error;
	asio::read(socket, response);

	//if (error != asio::error::eof)
	//	return "";

	std::istream response_stream(&response);
	std::string message;
	std::string outMessage;
	std::string fullmsg;

	bool bodyStarted = false;

	while (std::getline(response_stream, message))
	{
		fullmsg += message;

		if (bodyStarted)
		{
			if (message.length() > 10)
			{
				if (!outMessage.empty() && outMessage.back() == '\r')
					outMessage.resize(outMessage.size() - 1);

				outMessage += message;
				outHistory.push_back(message);
			}
		}

		if (message == "\r")
			bodyStarted = true;

		history.push_back(message);
		//std::cout << message << "\n";
	}

	outFHistory.push_back(outMessage);

	return fullmsg;
}

std::string sendHttpsRequest(ssl_socket& socket, tcp::resolver& resolver, asio::streambuf& request, const char* hostname)
{
	openSslSocket(socket, resolver, hostname);

	return sendHttpsRequest(socket, request);
}
#endif

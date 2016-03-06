#include <Network.h>
#include "boost\asio\buffer.hpp"
#include "boost\array.hpp"

void openSslSocket(ssl_socket& sock, tcp::resolver& resolver, const char* server)
{
	tcp::resolver::query query(server, "https");
	boost::asio::connect(sock.lowest_layer(), resolver.resolve(query));
	sock.lowest_layer().set_option(tcp::no_delay(true));

	// Perform SSL handshake and verify the remote host's
	// certificate.
	//sock.set_verify_mode(ssl::verify_peer);
	//sock.set_verify_callback(ssl::rfc2818_verification(server));

	sock.set_verify_mode(ssl::verify_none);
	sock.handshake(ssl_socket::client);
}

std::vector<std::string> outFHistory;
std::vector<std::string> outHistory;
std::vector<std::string> history;

std::string sendHttpsRequest(ssl_socket& socket, tcp::resolver& resolver, boost::asio::streambuf& request, const char* server)
{
	openSslSocket(socket, resolver, server);

	// Send the request.
	boost::asio::write(socket, request);

	// Read the response status line. The response streambuf will automatically
	// grow to accommodate the entire line. The growth may be limited by passing
	// a maximum size to the streambuf constructor.
	boost::asio::streambuf response;
	boost::system::error_code error;
	boost::asio::read(socket, response, error);

	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	std::istream response_stream(&response);
	std::string message;
	std::string outMessage;

	bool bodyStarted = false;

	while (std::getline(response_stream, message))
	{
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

	return outMessage;
}


std::vector<char> sendUdpRequest(udp::socket& socket, udp::resolver& resolver, std::vector<char> request, const char* server, const char* port)
{
	udp::resolver::query query(udp::v4(), server, port);
	udp::endpoint receiver_endpoint = *resolver.resolve(query);

	socket.send_to(boost::asio::buffer(request), receiver_endpoint);

	boost::array<char, 10*1024> recv_buf;
	udp::endpoint sender_endpoint;
	size_t len = socket.receive_from( boost::asio::buffer(recv_buf), sender_endpoint);

	std::vector<char> out(recv_buf.data(), recv_buf.data() + len);
	return out;
}

void openTcpSocket(tcp::socket& socket, tcp::resolver& resolver, const char* server, const char* port)
{
	tcp::resolver::query query(server, port);
	
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && endpoint_iterator != end)
	{
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}
	if (error)
		throw boost::system::system_error(error);
}

std::vector<char> sendTcpRequest(tcp::socket& socket, std::vector<char>& request)
{
	boost::asio::streambuf sBuffer;
	sBuffer.sputn(request.data(), request.size());

	// Send the request.
	boost::asio::write(socket, sBuffer);

	/*boost::asio::streambuf response;
	boost::system::error_code error;
	boost::asio::read(socket, response, error);

	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	std::istream response_stream(&response);
	std::string message;
	std::string outMessage;*/

	boost::asio::streambuf response;
	boost::system::error_code error;

	size_t len = boost::asio::read(socket, response, error);

	if (len > 0)
	{
		std::istream response_stream(&response);
		std::vector<char> buffer;

		buffer.resize(len);
		response_stream.read(&buffer[0], len);

		return buffer;
	}
	else if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	return{};
}

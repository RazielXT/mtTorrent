#pragma once

#include "RiotRestApi.h"
#include "utils\Network.h"
#include <fstream>

std::string tabs;

std::map<uint64_t, std::string> championsInfo;

void printJson(js::value& value, std::ostringstream& output);

void printChampion(js::value& value, std::ostringstream& output)
{
	if (value.is<double>())
	{
		uint64_t n = static_cast<uint64_t>(value.get<double>());
		output << tabs << championsInfo[n] << "\n";
	}
	else
		printJson(value, output);
}

void fillChampionInfo(js::object& map)
{
	if (map.find("id") != map.end() && map.find("name") != map.end())
	{
		auto& id = map["id"];
		auto& name = map["name"];

		if (id.is<double>() && name.is<std::string>())
		{
			championsInfo[static_cast<uint64_t>(id.get<double>())] = name.get<std::string>();
		}
	}
}

void printJson(js::value& value, std::ostringstream& output)
{
	if (value.is<js::null>())
		return;

	if (value.is<js::object>())
	{
		output << tabs << "{\n";

		std::string oldTabs = tabs;
		tabs += "\t";

		auto map = value.get<js::object>();
		fillChampionInfo(map);

		for (auto o : map)
		{
			output << tabs << o.first << " : ";

			if (o.first == "championId")
				printChampion(o.second, output);
			else
				printJson(o.second, output);
		}

		tabs = oldTabs;

		output << tabs << "}\n";
	}

	if (value.is<js::array>())
	{
		output << tabs << "[\n";

		std::string oldTabs = tabs;
		tabs += "\t";

		auto array = value.get<js::array>();
		for (auto o : array)
		{
			printJson(o, output);
		}

		tabs = oldTabs;

		output << tabs << "]\n";
	}

	if (value.is<std::string>())
		output << tabs << value.get<std::string>() << "\n";

	if (value.is<double>())
	{
		uint64_t n = static_cast<uint64_t>(value.get<double>());
		output << tabs << std::to_string(n) << "\n";
	}


	if (value.is<bool>())
		output << tabs << value.get<bool>() << "\n";
}

void riotApiRequest()
{
	char* server = "global.api.pvp.net";
	//char* req = "https://global.api.pvp.net/api/lol/eune/v1.2/champion?api_key=14f747df-2f28-4e11-ad45-3811773feed8";
	//char* req = "https://global.api.pvp.net/api/lol/eune/v1.4/summoner/by-name/Razielll?api_key=14f747df-2f28-4e11-ad45-3811773feed8";
	//char* req = "https://global.api.pvp.net/api/lol/eune/v1.4/summoner/30283005/masteries?api_key=14f747df-2f28-4e11-ad45-3811773feed8";
	char* recentReq = "https://global.api.pvp.net/api/lol/eune/v1.3/game/by-summoner/30283005/recent?api_key=14f747df-2f28-4e11-ad45-3811773feed8";
	//char* req = "https://global.api.pvp.net/api/lol/static-data/eune/v1.2/champion/420?api_key=14f747df-2f28-4e11-ad45-3811773feed8";
	char* champsReq = "https://global.api.pvp.net/api/lol/static-data/eune/v1.2/champion?api_key=14f747df-2f28-4e11-ad45-3811773feed8";

	// Create a context that uses the default paths for
	// finding CA certificates.
	ssl::context ctx(ssl::context::tlsv12);
	ctx.set_default_verify_paths();

	// Open a socket and connect it to the remote host.
	boost::asio::io_service io_service;
	ssl_socket sock(io_service, ctx);
	tcp::resolver resolver(io_service);


	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << champsReq << " HTTP/1.1\r\n";
	request_stream << "Host: " << server << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	auto message = sendHttpsRequest(sock, resolver, request, server);

	js::value v;
	std::string err = js::parse(v, message);
	std::ostringstream out;

	if (err.empty())
		printJson(v, out);

	//-----------------------

	boost::asio::streambuf request2;
	std::ostream request_stream2(&request2);
	request_stream2 << "GET " << recentReq << " HTTP/1.1\r\n";
	request_stream2 << "Host: " << server << "\r\n";
	request_stream2 << "Accept: */*\r\n";
	request_stream2 << "Connection: close\r\n\r\n";

	message = sendHttpsRequest(sock, resolver, request2, server);

	err = js::parse(v, message);
	std::ostringstream out2;

	if (err.empty())
		printJson(v, out2);

	std::ofstream file("D:\\out.txt");
	file << out2.str();
}
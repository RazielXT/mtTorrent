#pragma once

#include "Interface.h"
#include "TcpAsyncStream.h"

namespace mtt
{
	class HttpTrackerComm
	{
	public:

		HttpTrackerComm();
		~HttpTrackerComm();

		void startTracker(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* torrent);

		std::function<void(AnnounceResponse&)> onAnnounceResponse;

	private:

		void onTcpClosed();
		void onTcpConnected();
		void onTcpReceived();

		std::string hostname;
		std::string port;
		std::shared_ptr<TcpAsyncStream> tcpComm;

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		TorrentFileInfo* torrent;
	};
}
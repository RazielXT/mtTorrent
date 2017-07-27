#pragma once

#include "Interface.h"
#include "TcpAsyncStream.h"
#include "ITracker.h"

namespace mtt
{
	class HttpTrackerComm : public Tracker
	{
	public:

		HttpTrackerComm();
		~HttpTrackerComm();

		void start(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* torrent);

	private:

		void onTcpClosed();
		void onTcpConnected();
		void onTcpReceived();

		std::string port;
		std::shared_ptr<TcpAsyncStream> tcpComm;

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

	};
}
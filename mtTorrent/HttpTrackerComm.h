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

		virtual void init(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* torrent) override;

		virtual void announce() override;

	private:

		void fail();

		void onTcpClosed();
		void onTcpConnected();
		void onTcpReceived();

		std::string port;
		std::shared_ptr<TcpAsyncStream> tcpComm;

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		uint32_t readAnnounceResponse(DataBuffer& buffer, AnnounceResponse& out);

		struct HttpHeaderInfo
		{
			bool valid = true;
			bool success = false;
			uint32_t dataStart = 0;
			uint32_t dataSize = 0;

			std::vector<std::pair<std::string, std::string>> headerParameters;
		};
		HttpHeaderInfo readHttpHeader(DataBuffer& buffer);

	};
}
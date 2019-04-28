#pragma once

#include "Interface.h"
#include "utils/TcpAsyncStream.h"
#include "ITracker.h"

namespace mtt
{
	class HttpTrackerComm : public Tracker
	{
	public:

		HttpTrackerComm();
		~HttpTrackerComm();

		virtual void init(std::string host, std::string port, TorrentPtr core) override;

		virtual void announce() override;

	private:

		void fail();

		void onTcpClosed(int code);
		void onTcpConnected();
		void onTcpReceived();

		std::string port;
		std::shared_ptr<TcpAsyncStream> tcpComm;

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		uint32_t readAnnounceResponse(DataBuffer& buffer, AnnounceResponse& out);

	};
}
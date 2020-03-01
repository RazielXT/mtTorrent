#pragma once

#include "Interface.h"
#include "utils\Network.h"
#include "ITracker.h"

#ifdef MTT_WITH_SSL

namespace mtt
{
	class HttpsTrackerComm : public Tracker
	{
	public:

		HttpsTrackerComm();
		~HttpsTrackerComm();

		virtual void init(std::string host, std::string port, TorrentPtr core) override;
		virtual void deinit() override;

		virtual void announce() override;

	private:

		void initializeStream();
		void fail();

		void onTcpClosed(int code);
		void onTcpConnected();
		void onTcpReceived(std::string&);

		std::string urlpath;
		std::shared_ptr<ssl_socket> socket;
		asio::ssl::context ctx;

		DataBuffer createAnnounceRequest(std::string host);
		uint32_t readAnnounceResponse(std::string& buffer, AnnounceResponse& out);

	};
}

#endif // MTT_WITH_SSL

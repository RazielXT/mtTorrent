#pragma once

#include "Interface.h"
#include "utils\Network.h"
#include "HttpTrackerComm.h"

#ifdef MTT_WITH_SSL

namespace mtt
{
	class HttpsTrackerComm : public HttpTracker
	{
	public:

		HttpsTrackerComm();
		~HttpsTrackerComm();

		virtual void init(std::string host, std::string port, std::string path, TorrentPtr core) override;
		virtual void deinit() override;

		virtual void announce() override;

	private:

		bool initializeStream();
		void fail();

		void onTcpReceived(std::string&);

		std::shared_ptr<ssl_socket> socket;
		asio::ssl::context ctx;
	};
}

#endif // MTT_WITH_SSL

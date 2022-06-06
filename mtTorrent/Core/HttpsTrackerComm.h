#pragma once

#include "Interface.h"
#include "utils/SslAsyncStream.h"
#include "HttpTrackerComm.h"

#ifdef MTT_WITH_SSL

namespace mtt
{
	class HttpsTrackerComm : public HttpTracker
	{
	public:

		HttpsTrackerComm();
		~HttpsTrackerComm();

		void init(std::string host, std::string port, std::string path, TorrentPtr core) override;
		void deinit() override;

		void announce() override;

	private:

		void fail();

		void onTcpReceived(const BufferView&);

		std::mutex commMutex;
		std::shared_ptr<SslAsyncStream> stream;
	};
}

#endif // MTT_WITH_SSL

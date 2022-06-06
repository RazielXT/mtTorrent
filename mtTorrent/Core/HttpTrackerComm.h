#pragma once

#include "Interface.h"
#include "utils/TcpAsyncStream.h"
#include "ITracker.h"

namespace mtt
{
	class HttpTracker : public Tracker
	{
	protected:

		virtual ~HttpTracker() = default;

		DataBuffer createAnnounceRequest(std::string path, std::string host, std::string port);
		uint32_t readAnnounceResponse(const char* buffer, size_t bufferSize, AnnounceResponse& out);

	};

	class HttpTrackerComm : public HttpTracker, public std::enable_shared_from_this<HttpTrackerComm>
	{
	public:

		HttpTrackerComm();
		~HttpTrackerComm();

		void init(std::string host, std::string port, std::string path, TorrentPtr t) override;
		void deinit() override;

		void announce() override;

	private:

		void initializeStream();
		void fail();

		void onTcpClosed(int code);
		void onTcpConnected();
		size_t onTcpReceived(const BufferView&);

		std::shared_ptr<TcpAsyncStream> tcpComm;
		std::mutex commMutex;
	};
}
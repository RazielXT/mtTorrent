#pragma once

#include "Interface.h"
#include "utils/TcpAsyncStream.h"
#include "ITracker.h"

namespace mtt
{
	class HttpTracker : public Tracker
	{
	protected:

		DataBuffer createAnnounceRequest(std::string path, std::string host, std::string port);
		uint32_t readAnnounceResponse(const char* buffer, size_t bufferSize, AnnounceResponse& out);

	};

	class HttpTrackerComm : public HttpTracker
	{
	public:

		HttpTrackerComm();
		~HttpTrackerComm();

		virtual void init(std::string host, std::string port, std::string path, TorrentPtr core) override;
		virtual void deinit() override;

		virtual void announce() override;

	private:

		void initializeStream();
		void fail();

		void onTcpClosed(int code);
		void onTcpConnected();
		size_t onTcpReceived(const BufferView&);

		std::shared_ptr<TcpAsyncStream> tcpComm;

	};
}
#pragma once
#include "Interface.h"

class ExternalRequestHandler
{
public:

	void openMagnetLink(uint32_t requestId, std::string& link, std::function<void(bool, mtt::TorrentPtr)> onResult);

	void stopRequest(uint32_t requestId);

private:

	struct RequestTask
	{
		virtual void stop() = 0;
		uint32_t requestId;
	};

	std::vector<std::shared_ptr<RequestTask>> tasks;

};
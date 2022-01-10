#include "ServiceThreadpool.h"

ServiceThreadpool::ServiceThreadpool()
{
}

ServiceThreadpool::ServiceThreadpool(uint32_t startWorkers)
{
	if(startWorkers)
		start(startWorkers);
}

ServiceThreadpool::~ServiceThreadpool()
{
	stop();
}

void ServiceThreadpool::start(uint32_t startWorkers)
{
	if(!work)
		work = std::make_shared<asio::io_service::work>(io);

	if (workers >= startWorkers)
		return;

	startWorkers = std::min(startWorkers, (uint32_t)std::size(myThreads));

	for (size_t i = workers; i < startWorkers; i++)
	{
		myThreads[i] = std::thread([this]() { io.run(); });
	}

	workers = std::max(startWorkers, workers);
}

void ServiceThreadpool::stop()
{
	if (work)
		work = nullptr;

	io.stop();

	for (size_t i = 0; i < workers; i++)
	{
		myThreads[i].join();
	}

	io.reset();

	workers = 0;
}

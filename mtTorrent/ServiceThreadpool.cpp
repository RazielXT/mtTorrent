#include "ServiceThreadpool.h"

ServiceThreadpool::ServiceThreadpool()
{
	start(1);
}

ServiceThreadpool::~ServiceThreadpool()
{
	stop();
}

void ServiceThreadpool::start(uint32_t startWorkers)
{
	if(!work)
		work = std::make_shared<boost::asio::io_service::work>(io);

	if (workers >= startWorkers)
		return;

	startWorkers = std::min(startWorkers, (uint32_t)_countof(myThreads));

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

	workers = 0;
}

void ServiceThreadpool::adjust(uint32_t estimatedWork)
{
	uint32_t workers = ((uint32_t)estimatedWork*0.26f) + 2;
	start(workers);
}

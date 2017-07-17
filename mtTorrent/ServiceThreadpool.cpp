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

	startWorkers = std::min(startWorkers, (uint32_t)4);

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

	for (size_t i = 0; i < workers; i++)
	{
		myThreads[i].join();
	}

	workers = 0;
}

void ServiceThreadpool::adjust(uint32_t estimatedWork)
{

}

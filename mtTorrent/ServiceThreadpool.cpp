#include "ServiceThreadpool.h"
#include <thread>

ServiceThreadpool::ServiceThreadpool() : work(io)
{
	start(1);
}

void ServiceThreadpool::start(uint32_t startWorkers)
{
	if (workers >= startWorkers)
		return;

	uint32_t moreWorkers = startWorkers - workers;

	for (size_t i = 0; i < moreWorkers; i++)
	{
		std::thread([this]() { io.run(); }).detach();
	}

	workers = std::min(startWorkers, workers);
}

void ServiceThreadpool::stop()
{

}

void ServiceThreadpool::adjust(uint32_t estimatedWork)
{

}

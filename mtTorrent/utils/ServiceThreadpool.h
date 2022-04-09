#pragma once

#include <asio.hpp>
#include <memory>
#include <thread>

class ServiceThreadpool
{
public:

	ServiceThreadpool();
	ServiceThreadpool(uint32_t startWorkers);
	~ServiceThreadpool();

	asio::io_service io;

	void start(uint32_t startWorkers);
	void stop();

private:

	using workType = asio::executor_work_guard<asio::io_context::executor_type>;
	std::shared_ptr<workType> work;

	std::thread myThreads[10];
	uint32_t workers = 0;
};
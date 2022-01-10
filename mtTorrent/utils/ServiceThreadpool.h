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

	std::shared_ptr<asio::io_service::work> work;

	std::thread myThreads[10];
	uint32_t workers = 0;
};
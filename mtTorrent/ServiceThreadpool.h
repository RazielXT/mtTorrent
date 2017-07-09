#pragma once

#include <boost/asio.hpp>

class ServiceThreadpool
{
public:

	boost::asio::io_service io_service;

	void start(uint32_t startWorkers);
	void stop();

	void adjust(uint32_t estimatedWork);

private:

	boost::asio::io_service::work work;

	void workerThread();
	uint32_t workers = 0;
};
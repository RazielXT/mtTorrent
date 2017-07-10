#pragma once

#include <boost/asio.hpp>

class ServiceThreadpool
{
public:

	ServiceThreadpool();

	boost::asio::io_service io;

	void start(uint32_t startWorkers);
	void stop();

	void adjust(uint32_t estimatedWork);

private:

	boost::asio::io_service::work work;

	uint32_t workers = 0;
};
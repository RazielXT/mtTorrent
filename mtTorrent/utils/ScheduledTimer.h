#pragma once
#include <boost/asio.hpp>
#include <functional>

struct ScheduledTimer : public std::enable_shared_from_this<ScheduledTimer>
{
	static std::shared_ptr<ScheduledTimer> create(boost::asio::io_service& io, std::function<void()> callback);

	ScheduledTimer(boost::asio::io_service& io, std::function<void()> callback);
	~ScheduledTimer();

	void schedule(uint32_t secondsOffset);
	void disable();
	uint32_t getSecondsTillNextUpdate();

private:

	void checkTimer();
	std::function<void()> func;
	boost::asio::deadline_timer timer;
};
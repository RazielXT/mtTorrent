#include "ScheduledTimer.h"

ScheduledTimer::ScheduledTimer(boost::asio::io_service& io, std::function<void()> callback) : timer(io), func(callback)
{
}

void ScheduledTimer::schedule(uint32_t secondsOffset)
{
	timer.async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this()));
	timer.expires_from_now(boost::posix_time::seconds(secondsOffset));
}

void ScheduledTimer::disable()
{
	func = nullptr;
	timer.expires_at(boost::posix_time::pos_infin);
}

uint32_t ScheduledTimer::getSecondsTillNextUpdate()
{
	if (timer.expires_at().is_infinity())
		return 0;
	else if (timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
		return 0;
	else
	{
		auto time = timer.expires_at() - boost::asio::deadline_timer::traits_type::now();
		return (uint32_t)time.total_seconds();
	}
}

void ScheduledTimer::checkTimer()
{
	if (timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		timer.expires_at(boost::posix_time::pos_infin);

		if (func)
			func();
	}

	timer.async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this()));
}

std::shared_ptr<ScheduledTimer> ScheduledTimer::create(boost::asio::io_service& io, std::function<void()> callback)
{
	return std::make_shared<ScheduledTimer>(io, callback);
}

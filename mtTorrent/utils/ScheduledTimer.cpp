#include "ScheduledTimer.h"

ScheduledTimer::ScheduledTimer(boost::asio::io_service& io, std::function<void()> callback) : func(callback)
{
	timer = std::make_unique<boost::asio::deadline_timer>(io);
}

ScheduledTimer::~ScheduledTimer()
{
	disable();
}

void ScheduledTimer::schedule(uint32_t secondsOffset)
{
	std::lock_guard<std::mutex> guard(mtx);

	if (timer)
	{
		timer->expires_from_now(boost::posix_time::seconds(secondsOffset));
		timer->async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this()));
	}
}

void ScheduledTimer::disable()
{
	std::lock_guard<std::mutex> guard(mtx);

	if (timer)
	{
		boost::system::error_code ec;
		timer->cancel(ec);
		timer.reset();
	}

	func = nullptr;
}

uint32_t ScheduledTimer::getSecondsTillNextUpdate()
{
	if (!timer || timer->expires_at().is_infinity())
		return 0;
	else if (timer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
		return 0;
	else
	{
		auto time = timer->expires_at() - boost::asio::deadline_timer::traits_type::now();
		return (uint32_t)time.total_seconds();
	}
}

void ScheduledTimer::checkTimer()
{
	std::function<void()> runFunc;

	{
		std::lock_guard<std::mutex> guard(mtx);

		if (timer)
		{
			if (timer->expires_at() <= boost::asio::deadline_timer::traits_type::now())
			{
				timer->expires_at(boost::posix_time::pos_infin);

				runFunc = func;
			}
			else
				timer->async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this()));
		}
	}

	if (runFunc)
		runFunc();
}

std::shared_ptr<ScheduledTimer> ScheduledTimer::create(boost::asio::io_service& io, std::function<void()> callback)
{
	return std::make_shared<ScheduledTimer>(io, callback);
}

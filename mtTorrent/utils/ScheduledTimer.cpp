#include "ScheduledTimer.h"

ScheduledTimer::ScheduledTimer(asio::io_service& io, std::function<Duration()> callback) : func(callback)
{
	timer = std::make_unique<asio::steady_timer>(io);
}

ScheduledTimer::~ScheduledTimer()
{
	disable();
}

void ScheduledTimer::schedule(Duration time)
{
	std::lock_guard<std::mutex> guard(mtx);

	scheduleInternal(time);
}

void ScheduledTimer::disable()
{
	std::lock_guard<std::mutex> guard(mtx);

	if (timer)
	{
		std::error_code ec;
		timer->cancel(ec);
		timer.reset();
	}

	func = nullptr;
}

void ScheduledTimer::scheduleInternal(Duration time)
{
	if (timer)
	{
		timer->expires_after(time);
		timer->async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this(), std::placeholders::_1));
	}
}

void ScheduledTimer::checkTimer(const asio::error_code& error)
{
	std::lock_guard<std::mutex> guard(mtx);

	if (!error && timer && func)
	{
		auto next = func();
		if (next != Duration(0))
			scheduleInternal(next);
	}
}

std::shared_ptr<ScheduledTimer> ScheduledTimer::create(asio::io_service& io, std::function<Duration()> callback)
{
	return std::make_shared<ScheduledTimer>(io, callback);
}

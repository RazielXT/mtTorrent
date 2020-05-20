#include "ScheduledTimer.h"

ScheduledTimer::ScheduledTimer(asio::io_service& io, std::function<void()> callback) : func(callback)
{
	timer = std::make_unique<asio::steady_timer>(io);
}

ScheduledTimer::~ScheduledTimer()
{
	disable();
}

void ScheduledTimer::schedule(uint32_t secondsOffset)
{
	schedule(std::chrono::seconds(secondsOffset));
}

void ScheduledTimer::schedule(std::chrono::milliseconds time)
{
	std::lock_guard<std::mutex> guard(mtx);

	if (timer)
	{
		timer->expires_from_now(time);
		timer->async_wait(std::bind(&ScheduledTimer::checkTimer, shared_from_this(), std::placeholders::_1));
	}
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

uint32_t ScheduledTimer::getSecondsTillNextUpdate()
{
	if (!timer)
		return 0;
	else
	{
		auto now = std::chrono::steady_clock::now();
		auto expTime = timer->expires_at();

		if (expTime < now)
			return 0;
		else

			return std::chrono::duration_cast<std::chrono::duration<uint32_t>>(expTime - now).count();
	}
}

void ScheduledTimer::checkTimer(const asio::error_code& error)
{
	std::function<void()> runFunc;

	{
		std::lock_guard<std::mutex> guard(mtx);

		if (!error && timer)
		{
			runFunc = func;
		}
	}

	if (runFunc)
		runFunc();
}

std::shared_ptr<ScheduledTimer> ScheduledTimer::create(asio::io_service& io, std::function<void()> callback)
{
	return std::make_shared<ScheduledTimer>(io, callback);
}

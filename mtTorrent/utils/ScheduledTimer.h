#pragma once

#include <asio.hpp>
#include <functional>
#include <chrono>

struct ScheduledTimer : public std::enable_shared_from_this<ScheduledTimer>
{
	using Duration = std::chrono::milliseconds;
	static std::shared_ptr<ScheduledTimer> create(asio::io_service& io, std::function<Duration()> callback);

	ScheduledTimer(asio::io_service& io, std::function<std::chrono::milliseconds()> callback);
	~ScheduledTimer();

	void schedule(Duration time);

	void disable();

private:

	void scheduleInternal(Duration time);

	void checkTimer(const asio::error_code& error);
	std::function<Duration()> func;
	std::unique_ptr<asio::steady_timer> timer;
	std::mutex mtx;
};
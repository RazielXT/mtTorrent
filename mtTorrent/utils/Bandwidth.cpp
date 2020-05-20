#include "Bandwidth.h"
#include <algorithm>
#include <map>

BandwidthChannel::BandwidthChannel()
{}

void BandwidthChannel::setLimit(int const l)
{
	limit = l;
}

int BandwidthChannel::getQuotaLeft() const
{
	if (limit == 0) return inf;
	return std::max(int(quotaLeft), 0);
}

void BandwidthChannel::updateQuota(int const dt_milliseconds)
{
	if (limit == 0) return;

	std::int64_t const toAdd = (std::int64_t(limit) * dt_milliseconds + 500) / 1000;

	if (toAdd > inf - quotaLeft)
	{
		quotaLeft = inf;
	}
	else
	{
		quotaLeft += toAdd;
		if (quotaLeft / 3 > limit) quotaLeft = std::int64_t(limit) * 3;
		quotaLeft = std::min(quotaLeft, std::int64_t(inf));
	}

	distributeQuota = int(std::max(quotaLeft, std::int64_t(0)));
}

void BandwidthChannel::returnQuota(int const amount)
{
	if (limit == 0) return;
	quotaLeft += amount;
}

void BandwidthChannel::useQuota(int const amount)
{
	if (limit == 0) return;

	quotaLeft -= amount;
}


BandwidthRequest::BandwidthRequest(std::shared_ptr<BandwidthUser> pe
	, int blk, int prio)
	: user(std::move(pe))
	, priority(prio)
	, requestSize(blk)
{
	memset(channel, 0, sizeof(channel));
}

int BandwidthRequest::assign_bandwidth()
{
	int quota = requestSize - assigned;

	if (quota == 0) return quota;

	for (int j = 0; j < MaxBandwidthChannels && channel[j]; ++j)
	{
		if (channel[j]->getLimit() == 0) continue;
		if (channel[j]->tmpPrioritySum == 0) continue;
		quota = std::min(int(std::int64_t(channel[j]->distributeQuota)
			* priority / channel[j]->tmpPrioritySum), quota);
	}
	assigned += quota;
	for (int j = 0; j < MaxBandwidthChannels && channel[j]; ++j)
		channel[j]->useQuota(quota);

	return quota;
}

BandwidthManager mgr;

BandwidthManager& BandwidthManager::Get()
{
	return mgr;
}

BandwidthChannel* BandwidthManager::GetChannel(const std::string& name)
{
	std::lock_guard<std::mutex> guard(requestMutex);

	auto it = channels.find(name);
	if (it != channels.end())
		return it->second.get();

	auto uptr = std::make_unique<BandwidthChannel>();
	auto ptr = uptr.get();
	channels[name] = std::move(uptr);

	return ptr;
}

BandwidthManager::BandwidthManager()
{
}

void BandwidthManager::close()
{
	abort = true;

	std::vector<BandwidthRequest> queue;
	queue.swap(requestsQueue);
	queuedBytes = 0;

	while (!queue.empty())
	{
		BandwidthRequest& bwr = queue.back();
		bwr.user->assignBandwidth(bwr.assigned);
		queue.pop_back();
	}
}

int BandwidthManager::queueSize() const
{
	return int(requestsQueue.size());
}

std::int64_t BandwidthManager::getQueuedBytes() const
{
	return queuedBytes;
}

int BandwidthManager::requestBandwidth(std::shared_ptr<BandwidthUser> peer, int const amount, int const priority, BandwidthChannel** chan, int const num_channels)
{
	if (abort)
		return 0;

	if (num_channels == 0)
		return amount;

	std::lock_guard<std::mutex> guard(requestMutex);

	int k = 0;
	BandwidthRequest bwr(std::move(peer), amount, priority);
	for (int i = 0; i < num_channels && chan[i]; ++i)
	{
		if (chan[i]->needQueueing(amount))
			bwr.channel[k++] = chan[i];
	}

	if (k == 0) return amount;

	queuedBytes += amount;
	requestsQueue.push_back(std::move(bwr));
	return 0;
}

void BandwidthManager::updateQuotas(int dt)
{
	if (abort) return;
	if (requestsQueue.empty()) return;

	if (dt > 3000) dt = 3000;

	std::vector<BandwidthChannel*> channels;

	std::vector<BandwidthRequest> queue;

	{
		std::lock_guard<std::mutex> guard(requestMutex);

		for (auto i = requestsQueue.begin(); i != requestsQueue.end();)
		{
			if (!i->user->isActive())
			{
				queuedBytes -= i->requestSize - i->assigned;

				// return all assigned quota to all the
				// bandwidth channels this peer belongs to
				for (int j = 0; j < BandwidthRequest::MaxBandwidthChannels && i->channel[j]; ++j)
				{
					BandwidthChannel* bwc = i->channel[j];
					bwc->returnQuota(i->assigned);
				}

				i->assigned = 0;
				queue.push_back(std::move(*i));
				i = requestsQueue.erase(i);
				continue;
			}
			for (int j = 0; j < BandwidthRequest::MaxBandwidthChannels && i->channel[j]; ++j)
			{
				BandwidthChannel* bwc = i->channel[j];
				bwc->tmpPrioritySum = 0;
			}
			++i;
		}

		for (auto const& r : requestsQueue)
		{
			for (int j = 0; j < BandwidthRequest::MaxBandwidthChannels && r.channel[j]; ++j)
			{
				BandwidthChannel* bwc = r.channel[j];
				if (bwc->tmpPrioritySum == 0) channels.push_back(bwc);
				bwc->tmpPrioritySum += r.priority;
			}
		}

		for (auto const& ch : channels)
		{
			ch->updateQuota(dt);
		}

		for (auto r = requestsQueue.begin(); r != requestsQueue.end();)
		{
			r->ttl -= dt;
			int a = r->assign_bandwidth();
			if (r->assigned == r->requestSize
				|| (r->ttl <= 0 && r->assigned > 0))
			{
				a += r->requestSize - r->assigned;
				queue.push_back(std::move(*r));
				r = requestsQueue.erase(r);
			}
			else
			{
				++r;
			}
			queuedBytes -= a;
		}
	}	

	while (!queue.empty())
	{
		BandwidthRequest& bwr = queue.back();
		bwr.user->assignBandwidth(bwr.assigned);
		queue.pop_back();
	}
}

#include "Bandwidth.h"
#include <algorithm>
#include <map>
#include <cstring>
#include "Logging.h"

#define BW_LOG(x) WRITE_GLOBAL_LOG(Bandwidth, x)

BandwidthChannel::BandwidthChannel()
{
}

void BandwidthChannel::setLimit(int const l)
{
	limit = l;
}

int BandwidthChannel::getLimit() const
{
	return limit;
}

void BandwidthChannel::updateQuota(int const dt_milliseconds)
{
	if (limit == 0) return;

	const std::int64_t toAdd = (std::int64_t(limit) * dt_milliseconds + 500) / 1000;

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

	BW_LOG("updateQuota channel toAdd" << toAdd << ", distributeQuota " << distributeQuota << ", quotaLeft " << quotaLeft);
}

void BandwidthChannel::returnQuota(int const amount)
{
	if (limit == 0) return;
	quotaLeft += amount;

	BW_LOG("returnQuota " << amount << ", quotaLeft " << quotaLeft);
}

void BandwidthChannel::useQuota(int const amount)
{
	if (limit == 0) return;
	quotaLeft -= amount;

	BW_LOG("useQuota " << amount << ", quotaLeft " << quotaLeft);
}

bool BandwidthChannel::needQueueing(int amount)
{
	if (limit == 0) return false;
	if (quotaLeft - amount < limit) return true;

	quotaLeft -= amount;

	return false;
}

BandwidthRequest::BandwidthRequest(std::shared_ptr<BandwidthUser> pe
	, int blk, int prio)
	: user(std::move(pe))
	, priority(prio)
	, requestSize(blk)
{
	BW_LOG(user->name() << "Request " << requestSize);
	memset(channel, 0, sizeof(channel));
}

int BandwidthRequest::assignBandwidth()
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

	BW_LOG(user->name() << "assignBandwidth " << quota << " total " << assigned << " requested " << requestSize);

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

uint32_t BandwidthManager::requestBandwidth(std::shared_ptr<BandwidthUser> peer, uint32_t amount, uint32_t priority, BandwidthChannel** chan, uint32_t num_channels)
{
	if (abort)
		return 0;

	if (num_channels == 0)
		return amount;

	std::lock_guard<std::mutex> guard(requestMutex);

	uint32_t k = 0;
	BandwidthRequest bwr(std::move(peer), amount, priority);
	for (uint32_t i = 0; i < num_channels && chan[i]; ++i)
	{
		if (chan[i]->needQueueing(amount))
			bwr.channel[k++] = chan[i];
	}

	if (k == 0) { BW_LOG("return " << amount);  return amount; }

	queuedBytes += amount;
	BW_LOG("queuedBytes " << queuedBytes);
	requestsQueue.push_back(std::move(bwr));
	return 0;
}

void BandwidthManager::updateQuotas(uint32_t dt)
{
	if (abort) return;
	if (requestsQueue.empty()) return;

	if (dt > 3000) dt = 3000;

	std::vector<BandwidthChannel*> channels;

	std::vector<BandwidthRequest> queue;

	BW_LOG("updateQuotas dt " << dt);
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
					BW_LOG("returnQuota " << i->assigned);
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
			int a = r->assignBandwidth();
			if (r->assigned == r->requestSize || (r->assigned >= r->minimumAssign) || (r->ttl <= 0 && r->assigned > 0))
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
			BW_LOG("queuedBytes " << queuedBytes);
		}
	}	

	while (!queue.empty())
	{
		BandwidthRequest& bwr = queue.back();
		bwr.user->assignBandwidth(bwr.assigned);
		queue.pop_back();
	}

	BW_LOG("updateQuotas finish");
}

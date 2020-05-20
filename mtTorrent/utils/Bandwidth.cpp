#include "Bandwidth.h"
#include <algorithm>
#include <map>

BandwidthChannel::BandwidthChannel()
	: tmp(0)
	, distribute_quota(0)
	, m_quota_left(0)
	, m_limit(0)
{}

// 0 means infinite
void BandwidthChannel::throttle(int const limit)
{
	//TORRENT_ASSERT_VAL(limit >= 0, limit);
	// if the throttle is more than this, we might overflow
	//TORRENT_ASSERT_VAL(limit < inf, limit);
	m_limit = limit;
}

int BandwidthChannel::quota_left() const
{
	if (m_limit == 0) return inf;
	return std::max(int(m_quota_left), 0);
}

void BandwidthChannel::update_quota(int const dt_milliseconds)
{
	//TORRENT_ASSERT_VAL(m_limit >= 0, m_limit);
	//TORRENT_ASSERT_VAL(m_limit < inf, m_limit);

	if (m_limit == 0) return;

	// "to_add" should never have int64 overflow: "m_limit" contains < "<int>::max"
	std::int64_t const to_add = (std::int64_t(m_limit) * dt_milliseconds + 500) / 1000;

	if (to_add > inf - m_quota_left)
	{
		m_quota_left = inf;
	}
	else
	{
		m_quota_left += to_add;
		if (m_quota_left / 3 > m_limit) m_quota_left = std::int64_t(m_limit) * 3;
		// "m_quota_left" will never have int64 overflow but may exceed "<int>::max"
		m_quota_left = std::min(m_quota_left, std::int64_t(inf));
	}

	distribute_quota = int(std::max(m_quota_left, std::int64_t(0)));
}

// this is used when connections disconnect with
// some quota left. It's returned to its bandwidth
// channels.
void BandwidthChannel::return_quota(int const amount)
{
	//TORRENT_ASSERT(amount >= 0);
	if (m_limit == 0) return;
	//TORRENT_ASSERT(m_quota_left <= m_quota_left + amount);
	m_quota_left += amount;
}

void BandwidthChannel::use_quota(int const amount)
{
	//TORRENT_ASSERT(amount >= 0);
	//TORRENT_ASSERT(m_limit >= 0);
	if (m_limit == 0) return;

	m_quota_left -= amount;
}


BandwidthRequest::BandwidthRequest(std::shared_ptr<BandwidthUser> pe
	, int blk, int prio)
	: user(std::move(pe))
	, priority(prio)
	, assigned(0)
	, request_size(blk)
{
	memset(channel, 0, sizeof(channel));
	//TORRENT_ASSERT(priority > 0);
}

int BandwidthRequest::assign_bandwidth()
{
	//TORRENT_ASSERT(assigned < request_size);
	int quota = request_size - assigned;
	//TORRENT_ASSERT(quota >= 0);

	if (quota == 0) return quota;

	for (int j = 0; j < max_bandwidth_channels && channel[j]; ++j)
	{
		if (channel[j]->throttle() == 0) continue;
		if (channel[j]->tmp == 0) continue;
		quota = std::min(int(std::int64_t(channel[j]->distribute_quota)
			* priority / channel[j]->tmp), quota);
	}
	assigned += quota;
	for (int j = 0; j < max_bandwidth_channels && channel[j]; ++j)
		channel[j]->use_quota(quota);
	//TORRENT_ASSERT(assigned <= request_size);
	return quota;
}

BandwidthManager mgr;

BandwidthManager& BandwidthManager::Get()
{
	return mgr;
}


BandwidthChannel* BandwidthManager::GetChannel(const std::string& name)
{
	std::lock_guard<std::mutex> guard(request_mutex);

	auto it = channels.find(name);
	if (it != channels.end())
		return it->second.get();

	auto uptr = std::make_unique<BandwidthChannel>();
	auto ptr = uptr.get();
	channels[name] = std::move(uptr);

	return ptr;
}

BandwidthManager::BandwidthManager()
	: m_queued_bytes(0)
	, m_abort(false)
{
}

void BandwidthManager::close()
{
	m_abort = true;

	std::vector<BandwidthRequest> queue;
	queue.swap(m_queue);
	m_queued_bytes = 0;

	while (!queue.empty())
	{
		BandwidthRequest& bwr = queue.back();
		bwr.user->assignBandwidth(bwr.assigned);
		queue.pop_back();
	}
}

int BandwidthManager::queue_size() const
{
	return int(m_queue.size());
}

std::int64_t BandwidthManager::queued_bytes() const
{
	return m_queued_bytes;
}

// non prioritized means that, if there's a line for bandwidth,
// others will cut in front of the non-prioritized peers.
// this is used by web seeds
int BandwidthManager::request_bandwidth(std::shared_ptr<BandwidthUser> peer, int const blk, int const priority, BandwidthChannel** chan, int const num_channels)
{
	if (m_abort) return 0;

	//TORRENT_ASSERT(blk > 0);
	//TORRENT_ASSERT(priority > 0);

	// if this assert is hit, the peer is requesting more bandwidth before
	// being assigned bandwidth for an already outstanding request
	//TORRENT_ASSERT(!is_queued(peer.get()));

	if (num_channels == 0)
	{
		// the connection is not rate limited by any of its
		// bandwidth channels, or it doesn't belong to any
		// channels. There's no point in adding it to
		// the queue, just satisfy the request immediately
		return blk;
	}

	std::lock_guard<std::mutex> guard(request_mutex);

	int k = 0;
	BandwidthRequest bwr(std::move(peer), blk, priority);
	for (int i = 0; i < num_channels && chan[i]; ++i)
	{
		if (chan[i]->need_queueing(blk))
			bwr.channel[k++] = chan[i];
	}

	if (k == 0) return blk;

	m_queued_bytes += blk;
	m_queue.push_back(std::move(bwr));
	return 0;
}

void BandwidthManager::update_quotas(int dt)
{
	if (m_abort) return;
	if (m_queue.empty()) return;

	if (dt > 3000) dt = 3000;

	// for each bandwidth channel, call update_quota(dt)

	std::vector<BandwidthChannel*> channels;

	std::vector<BandwidthRequest> queue;

	{
		std::lock_guard<std::mutex> guard(request_mutex);

		for (auto i = m_queue.begin(); i != m_queue.end();)
		{
			if (!i->user->isActive())
			{
				m_queued_bytes -= i->request_size - i->assigned;

				// return all assigned quota to all the
				// bandwidth channels this peer belongs to
				for (int j = 0; j < BandwidthRequest::max_bandwidth_channels && i->channel[j]; ++j)
				{
					BandwidthChannel* bwc = i->channel[j];
					bwc->return_quota(i->assigned);
				}

				i->assigned = 0;
				queue.push_back(std::move(*i));
				i = m_queue.erase(i);
				continue;
			}
			for (int j = 0; j < BandwidthRequest::max_bandwidth_channels && i->channel[j]; ++j)
			{
				BandwidthChannel* bwc = i->channel[j];
				bwc->tmp = 0;
			}
			++i;
		}

		for (auto const& r : m_queue)
		{
			for (int j = 0; j < BandwidthRequest::max_bandwidth_channels && r.channel[j]; ++j)
			{
				BandwidthChannel* bwc = r.channel[j];
				if (bwc->tmp == 0) channels.push_back(bwc);
				//TORRENT_ASSERT(INT_MAX - bwc->tmp > r.priority);
				bwc->tmp += r.priority;
			}
		}

		for (auto const& ch : channels)
		{
			ch->update_quota(dt);
		}

		for (auto r = m_queue.begin(); r != m_queue.end();)
		{
			r->ttl -= dt;
			int a = r->assign_bandwidth();
			if (r->assigned == r->request_size
				|| (r->ttl <= 0 && r->assigned > 0))
			{
				a += r->request_size - r->assigned;
				//TORRENT_ASSERT(r->assigned <= r->request_size);
				queue.push_back(std::move(*r));
				r = m_queue.erase(r);
			}
			else
			{
				++r;
			}
			m_queued_bytes -= a;
		}
	}	

	while (!queue.empty())
	{
		BandwidthRequest& bwr = queue.back();
		bwr.user->assignBandwidth(bwr.assigned);
		queue.pop_back();
	}
}
/*
void bw_peer::assign_bandwidth(int channel, int amount)
{
	waiting_for_bw = false;
	m_quota += amount;

	if (is_disconnecting())
		return;
}

void bw_peer::check_read()
{
	if (is_disconnecting())
		return;

	//reserve buffer size

	//request bw
	{
		int wanted_transfer = 0;
		{
			const int tick_interval = std::max(1, m_settings.get_int(settings_pack::tick_interval));
			std::int64_t const download_rate = std::int64_t(m_statistics.download_rate()) * 3 / 2;

			wanted_transfer =  std::max({ m_outstanding_bytes + 30
					, m_recv_buffer.packet_bytes_remaining() + 30
					, int(download_rate * tick_interval / 1000) });
		}

		int bytes = std::max(wanted_transfer, bytes);

		// we already have enough quota
		if (m_quota >= bytes) return 0;

		// deduct the bytes we already have quota for
		bytes -= m_quota;

		int const ret = manager->request_bandwidth(self(), bytes, priority, channels.data(), c);

		if (ret == 0)
			waiting_for_bw = true;
		else
			m_quota += ret;
	}

	if (m_quota == 0)
		return;

	//int const max_receive = std::min(buffer_size, quota_left);

	//m_socket->async_read_some(
}
*/
#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <array>
#include <vector>
#include <mutex>
#include <map>

class BandwidthUser
{
public:

	virtual void assignBandwidth(int amount) = 0;

	virtual bool isActive() = 0;
};

struct BandwidthChannel
{
	static constexpr int inf = (std::numeric_limits<std::int32_t>::max)();

	BandwidthChannel();

	// 0 means infinite
	void throttle(int limit);
	int throttle() const
	{
		return m_limit;
	}

	int quota_left() const;
	void update_quota(int dt_milliseconds);

	// this is used when connections disconnect with
	// some quota left. It's returned to its bandwidth
	// channels.
	void return_quota(int amount);
	void use_quota(int amount);

	// this is an optimization. If there is more than one second
	// of quota built up in this channel, just apply it right away
	// instead of introducing a delay to split it up evenly. This
	// should especially help in situations where a single peer
	// has a capacity under the rate limit, but would otherwise be
	// held back by the latency of getting bandwidth from the limiter
	bool need_queueing(int amount)
	{
		if (m_limit == 0) return false;
		if (m_quota_left - amount < m_limit) return true;
		m_quota_left -= amount;
		return false;
	}

	// used as temporary storage while distributing
	// bandwidth
	int tmp;

	// this is the number of bytes to distribute this round
	int distribute_quota;

private:

	// this is the amount of bandwidth we have
	// been assigned without using yet.
	int64_t m_quota_left;

	// the limit is the number of bytes
	// per second we are allowed to use.
	int32_t m_limit;
};

struct BandwidthRequest
{
	BandwidthRequest(std::shared_ptr<BandwidthUser> user, int blk, int prio);

	std::shared_ptr<BandwidthUser> user;
	// 1 is normal prio
	int priority;
	// the number of bytes assigned to this request so far
	int assigned;
	// once assigned reaches this, we dispatch the request function
	int request_size;

	// the max number of ms for this request to survive
	// this ensures that requests gets responses at very low
	// rate limits, when the requested size would take a long
	// time to satisfy
	int ttl = 10000;

	// loops over the bandwidth channels and assigns bandwidth
	// from the most limiting one
	int assign_bandwidth();

	static constexpr int max_bandwidth_channels = 2;
	// we don't actually support more than 10 channels per peer
	BandwidthChannel* channel[max_bandwidth_channels];
};


struct BandwidthManager
{
	BandwidthManager();

	static BandwidthManager& Get();
	BandwidthChannel* GetChannel(const std::string& name);

	void close();

	int queue_size() const;
	std::int64_t queued_bytes() const;

	// non prioritized means that, if there's a line for bandwidth,
	// others will cut in front of the non-prioritized peers.
	// this is used by web seeds
	// returns the number of bytes to assign to the peer, or 0
	// if the peer's 'assign_bandwidth' callback will be called later
	int request_bandwidth(std::shared_ptr<BandwidthUser> peer, int blk, int priority, BandwidthChannel** chan, int num_channels);

	void update_quotas(int dt);

private:

	std::mutex request_mutex;

	// these are the consumers that want bandwidth
	std::vector<BandwidthRequest> m_queue;
	// the number of bytes all the requests in queue are for
	std::int64_t m_queued_bytes;

	bool m_abort;

	std::map<std::string, std::unique_ptr<BandwidthChannel>> channels;
};

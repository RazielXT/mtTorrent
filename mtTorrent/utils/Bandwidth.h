#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <array>
#include <vector>
#include <mutex>
#include <map>

//inspired by libtorrent bandwidth management

class BandwidthUser
{
public:

	virtual void assignBandwidth(int amount) = 0;

	virtual bool isActive() = 0;

	virtual std::string name() = 0;
};

struct BandwidthChannel
{
	static constexpr int inf = (std::numeric_limits<std::int32_t>::max)();

	BandwidthChannel();

	// 0 means infinite
	void setLimit(int limit);
	int getLimit() const;

	void updateQuota(int dtMilliseconds);

	// this is used when connections disconnect with
	// some quota left. It's returned to its bandwidth
	// channels.
	void returnQuota(int amount);
	void useQuota(int amount);

	// this is an optimization. If there is more than one second
	// of quota built up in this channel, just apply it right away
	// instead of introducing a delay to split it up evenly. This
	// should especially help in situations where a single peer
	// has a capacity under the rate limit, but would otherwise be
	// held back by the latency of getting bandwidth from the limiter
	bool needQueueing(int amount);

	// used as temporary storage while distributing
	// bandwidth
	int tmpPrioritySum = 0;

	// this is the number of bytes to distribute this round
	int distributeQuota = 0;

private:

	// this is the amount of bandwidth we have
	// been assigned without using yet.
	int64_t quotaLeft = 0;

	// the limit is the number of bytes
	// per second we are allowed to use.
	int32_t limit = 0;
};

struct BandwidthRequest
{
	BandwidthRequest(std::shared_ptr<BandwidthUser> user, int amount, int prio);

	std::shared_ptr<BandwidthUser> user;
	// 1 is normal prio
	int priority;
	int assigned = 0;
	int requestSize;

	// the max number of ms for this request to survive
	// this ensures that requests gets responses at very low
	// rate limits, when the requested size would take a long
	// time to satisfy
	int ttl = 10000;

	// loops over the bandwidth channels and assigns bandwidth
	// from the most limiting one
	int assignBandwidth();

	static constexpr int MaxBandwidthChannels = 2;
	BandwidthChannel* channel[MaxBandwidthChannels];
};


struct BandwidthManager
{
	BandwidthManager();

	static BandwidthManager& Get();
	BandwidthChannel* GetChannel(const std::string& name);

	void close();

	// returns the number of bytes to assign to the peer, or 0
	// if the peer's 'assign_bandwidth' callback will be called later
	uint32_t requestBandwidth(std::shared_ptr<BandwidthUser> peer, uint32_t blk, uint32_t priority, BandwidthChannel** chan, uint32_t num_channels);

	void updateQuotas(uint32_t dt);

private:

	std::mutex requestMutex;

	// these are the consumers that want bandwidth
	std::vector<BandwidthRequest> requestsQueue;
	// the number of bytes all the requests in queue are for
	std::int64_t queuedBytes = 0;

	bool abort = false;

	std::map<std::string, std::unique_ptr<BandwidthChannel>> channels;
};

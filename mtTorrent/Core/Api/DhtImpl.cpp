#include "Dht/Communication.h"

bool mttApi::Dht::active() const
{
	auto comm = static_cast<const mtt::dht::Communication*>(this);

	return comm->active();
}

uint32_t mttApi::Dht::getNodesCount() const
{
	auto comm = static_cast<const mtt::dht::Communication*>(this);

	return comm->getNodesCount();
}

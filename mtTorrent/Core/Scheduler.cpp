#include "Scheduler.h"

void mtt::Scheduler::handshakeFinished(PeerCommunication*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::connectionClosed(PeerCommunication*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::messageReceived(PeerCommunication*, PeerMessage&)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::extHandshakeFinished(PeerCommunication*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::progressUpdated(PeerCommunication*)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::onConnected(std::shared_ptr<PeerCommunication>, Addr&)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::onConnectFail(Addr&)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void mtt::Scheduler::onAddrReceived(std::vector<Addr>&)
{
	throw std::logic_error("The method or operation is not implemented.");
}


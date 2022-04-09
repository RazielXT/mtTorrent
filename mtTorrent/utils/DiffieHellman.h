#pragma once

#include "BigNumber.h"
#include "DataBuffer.h"

struct DiffieHellmanExchange
{
	DiffieHellmanExchange(uint32_t keySize);

	void createExchangeMessage(DataBuffer& buffer, const BigNumber& generator, const BigNumber& primes);
	void createSecretFromRemoteKey(BigNumber& sharedSecret, const uint8_t* key, const BigNumber& primes);

private:

	BigNumber privateKey;
};

struct PeDiffieHellmanExchange : protected DiffieHellmanExchange
{
	PeDiffieHellmanExchange();

	void createExchangeMessage(DataBuffer& buffer);
	void createSecretFromRemoteKey(BigNumber& sharedSecret, const uint8_t* key);

	static uint32_t KeySize();
};

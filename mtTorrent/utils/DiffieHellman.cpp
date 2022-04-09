#include "DiffieHellman.h"

DiffieHellmanExchange::DiffieHellmanExchange(uint32_t keySize)
{
	privateKey.SetByteSize(keySize);
	Random::Data(privateKey.GetDataBuffer(), keySize);
}

void DiffieHellmanExchange::createExchangeMessage(DataBuffer& buffer, const BigNumber& generator, const BigNumber& primes)
{
	//public key = (g ^ secret) mod prime
	BigNumber publicKey;
	BigNumber::Powm(publicKey, generator, privateKey, primes);

	publicKey.Export(buffer.data(), buffer.size());
}

void DiffieHellmanExchange::createSecretFromRemoteKey(BigNumber& sharedSecret, const uint8_t* key, const BigNumber& primes)
{
	BigNumber remoteKey(key, primes.GetByteSize());

	sharedSecret.SetByteSize(primes.GetByteSize());

	//shared_secret = (remote_pubkey ^ local_secret) mod prime
	BigNumber::Powm(sharedSecret, remoteKey, privateKey, primes);

	//shouldnt need it anymore
	privateKey.SetZero();
}

static constexpr unsigned char pePrimesArray[] = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xC9,0x0F,0xDA,0xA2,0x21,0x68,0xC2,0x34,
	0xC4,0xC6,0x62,0x8B,0x80,0xDC,0x1C,0xD1,
	0x29,0x02,0x4E,0x08,0x8A,0x67,0xCC,0x74,
	0x02,0x0B,0xBE,0xA6,0x3B,0x13,0x9B,0x22,
	0x51,0x4A,0x08,0x79,0x8E,0x34,0x04,0xDD,
	0xEF,0x95,0x19,0xB3,0xCD,0x3A,0x43,0x1B,
	0x30,0x2B,0x0A,0x6D,0xF2,0x5F,0x14,0x37,
	0x4F,0xE1,0x35,0x6D,0x6D,0x51,0xC2,0x45,
	0xE4,0x85,0xB5,0x76,0x62,0x5E,0x7E,0xC6,
	0xF4,0x4C,0x42,0xE9,0xA6,0x3A,0x36,0x21,
	0x00,0x00,0x00,0x00,0x00,0x09,0x05,0x63
};

static const BigNumber pePrimes(pePrimesArray, sizeof(pePrimesArray));
static const BigNumber peGenerator(2, sizeof(pePrimesArray));

PeDiffieHellmanExchange::PeDiffieHellmanExchange() : DiffieHellmanExchange(sizeof(pePrimesArray))
{
}

void PeDiffieHellmanExchange::createExchangeMessage(DataBuffer& buffer)
{
	buffer.resize(PeDiffieHellmanExchange::KeySize() + Random::Number() % 512);
	Random::Data(buffer, PeDiffieHellmanExchange::KeySize());

	DiffieHellmanExchange::createExchangeMessage(buffer, peGenerator, pePrimes);
}

void PeDiffieHellmanExchange::createSecretFromRemoteKey(BigNumber& sharedSecret, const uint8_t* key)
{
	DiffieHellmanExchange::createSecretFromRemoteKey(sharedSecret, key, pePrimes);
}

uint32_t PeDiffieHellmanExchange::KeySize()
{
	return sizeof(pePrimesArray);
}

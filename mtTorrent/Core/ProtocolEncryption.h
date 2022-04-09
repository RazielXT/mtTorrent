#pragma once

#include "utils/DiffieHellman.h"
#include "utils/SHA.h"
#include "utils/RC4.h"
#include <memory>
#include <mutex>
#include <map>


class ProtocolEncryption
{
public:

	void encrypt(uint8_t* out, size_t len);

	void decrypt(const uint8_t* data, size_t len, DataBuffer& out);
	void decrypt(uint8_t* data, size_t len);
	void decrypt(BufferSpan span);

	RC4 rcIn;
	RC4 rcOut;
};

class ProtocolEncryptionListener
{
public:

	void addTorrent(const uint8_t* torrent);
	void removeTorrent(const uint8_t* torrent);

	const uint8_t* findByPe3Hash(const uint8_t* data, const uint8_t* req3Hash);

private:

	struct TorrentInfo
	{
		uint8_t req2Hash[SHA_DIGEST_LENGTH];
	};
	std::map<const uint8_t*, TorrentInfo> torrents;
	std::mutex mtx;
};

class ProtocolEncryptionHandshake
{
public:

	ProtocolEncryptionHandshake() = default;

	DataBuffer initiate(const uint8_t* info, DataBuffer message);
	DataBuffer initiate();
	size_t readRemoteDataAsInitiator(const BufferView& data, DataBuffer& response);

	size_t readRemoteDataAsListener(BufferSpan data, DataBuffer& response, ProtocolEncryptionListener& l);

	bool started() const;
	bool established() const;
	bool failed() const;

	DataBuffer& getInitialBtMessage();
	const uint8_t* getTorrentHash() const;

	std::unique_ptr<ProtocolEncryption> pe;

	enum class Type { PlainText = 1, RC4 = 2 } type = Type::RC4;

private:

	// 	1 A->B : Diffie Hellman Ya, PadA
	// 	2 B->A : Diffie Hellman Yb, PadB
	// 	3 A->B : HASH('req1', S), HASH('req2', SKEY) xor HASH('req3', S), ENCRYPT(VC, crypto_provide, len(PadC), PadC, len(IA)), ENCRYPT(IA)
	// 	4 B->A : ENCRYPT(VC, crypto_select, len(padD), padD), ENCRYPT2(Payload Stream)
	// 	5 A->B : ENCRYPT2(Payload Stream)
	enum class Step { Pe1, Pe2, Pe3, Pe4, Established, Error } nextStep = Step::Pe1;

	//sent or received in pe3
	DataBuffer pe3BtMessage;

	static constexpr size_t Pe3DataSizePrePadding = 2 * SHA_DIGEST_LENGTH + 8 + 4 + 2;

	void createPe3Handshake(DataBuffer& buffer);
	size_t readPe3Handshake(const BufferView& data, ProtocolEncryptionListener& l);

	void createPe4Handshake(DataBuffer& buffer);
	size_t readPe4Handshake(const BufferView& data);

	void prepareEncryption(bool invoker);

	BigNumber sharedSecret;
	const uint8_t* infoHash = nullptr;

	PeDiffieHellmanExchange dh;
};
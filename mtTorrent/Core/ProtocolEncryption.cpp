#include "ProtocolEncryption.h"
#include "utils/PacketHelper.h"

void ProtocolEncryption::encrypt(uint8_t* out, size_t len)
{
	rcOut.encode(out, len);
}

void ProtocolEncryption::decrypt(const uint8_t* data, size_t len, DataBuffer& out)
{
	out.assign(data, data + len);
	decrypt(out.data(), len);
}

void ProtocolEncryption::decrypt(uint8_t* data, size_t len)
{
	rcIn.decode(data, len);
}

void ProtocolEncryption::decrypt(BufferSpan span)
{
	return decrypt(span.data, span.size);
}

void ProtocolEncryptionListener::addTorrent(const uint8_t* torrent)
{
	// HASH('req2', SKEY)
	uint8_t req2[4 + SHA_DIGEST_LENGTH] = { 'r', 'e', 'q', '2' };
	memcpy(req2 + 4, torrent, SHA_DIGEST_LENGTH);

	std::lock_guard<std::mutex> guard(mtx);
	auto& t = torrents[torrent];
	_SHA1(req2, 4 + SHA_DIGEST_LENGTH, t.req2Hash);
}

void ProtocolEncryptionListener::removeTorrent(const uint8_t* torrent)
{
	std::lock_guard<std::mutex> guard(mtx);

	auto it = torrents.find(torrent);
	if (it != torrents.end())
	{
		torrents.erase(it);
	}
}

const uint8_t* ProtocolEncryptionListener::findByPe3Hash(const uint8_t* data, const uint8_t* req3Hash)
{
	std::lock_guard<std::mutex> guard(mtx);

	for (const auto& t : torrents)
	{
		bool same = true;

		// HASH('req2', SKEY) xor HASH('req3', S)
		for (int i = 0; same && i < SHA_DIGEST_LENGTH; i++)
			same = (data[i] != (t.second.req2Hash[i] ^ req3Hash[i]));

		if (same)
			return t.first;
	}

	return nullptr;
}

DataBuffer ProtocolEncryptionHandshake::initiate(const uint8_t* info, DataBuffer message)
{
	infoHash = info;
	pe3BtMessage = std::move(message);
	nextStep = Step::Pe2;

	DataBuffer buffer;
	dh.createExchangeMessage(buffer);

	return buffer;
}

DataBuffer ProtocolEncryptionHandshake::initiate()
{
	DataBuffer buffer;
	dh.createExchangeMessage(buffer);

	return buffer;
}

size_t ProtocolEncryptionHandshake::readRemoteDataAsInitiator(const BufferView& data, DataBuffer& response)
{
	if (nextStep == Step::Pe2)
	{
		if (data.size < PeDiffieHellmanExchange::KeySize())
			return 0;

		dh.createSecretFromRemoteKey(sharedSecret, data.data);
		createPe3Handshake(response);
		nextStep = Step::Pe4;

		return data.size; //all data after key padding
	}

	if (nextStep == Step::Pe4)
	{
		return readPe4Handshake(data);
	}

	return 0;
}

size_t ProtocolEncryptionHandshake::readRemoteDataAsListener(BufferSpan data, DataBuffer& response, ProtocolEncryptionListener& l)
{
	if (nextStep == Step::Pe1)
	{
		if (data.size < PeDiffieHellmanExchange::KeySize())
			return 0;

		dh.createExchangeMessage(response);
		dh.createSecretFromRemoteKey(sharedSecret, data.data);
		nextStep = Step::Pe3;

		return data.size; //all data after key is padding
	}

	if (nextStep == Step::Pe3)
	{
		auto i = readPe3Handshake(data, l);

		if (nextStep != Step::Established)
			return i;

		createPe4Handshake(response);

		return i;
	}

	return 0;
}

bool ProtocolEncryptionHandshake::started() const
{
	return nextStep != Step::Pe1;
}

bool ProtocolEncryptionHandshake::established() const
{
	return nextStep == Step::Established;
}

bool ProtocolEncryptionHandshake::failed() const
{
	return nextStep == Step::Error;
}

DataBuffer& ProtocolEncryptionHandshake::getInitialBtMessage()
{
	return pe3BtMessage;
}

const uint8_t* ProtocolEncryptionHandshake::getTorrentHash() const
{
	return infoHash;
}

constexpr size_t Pe3DataSizePrePadding = 2 * SHA_DIGEST_LENGTH + 8 + 4 + 2;

void ProtocolEncryptionHandshake::createPe3Handshake(DataBuffer& buffer)
{
	uint32_t pad = Random::Number() % 512;

	buffer.resize(Pe3DataSizePrePadding + pad + 2 + pe3BtMessage.size());
	Random::Data(buffer.data() + Pe3DataSizePrePadding, pad);
	auto dataPtr = buffer.data();

	{
		uint8_t reqBuffer[4 + 96] = { 'r', 'e', 'q', '1' };
		sharedSecret.Export(reqBuffer + 4, 96);
		_SHA1(reqBuffer, sizeof(reqBuffer), dataPtr);

		reqBuffer[3] = '3';
		dataPtr += SHA_DIGEST_LENGTH;
		_SHA1(reqBuffer, sizeof(reqBuffer), dataPtr);

		reqBuffer[3] = '2';
		memcpy(reqBuffer + 4, infoHash, SHA_DIGEST_LENGTH);
		uint8_t req2hash[SHA_DIGEST_LENGTH];
		_SHA1(reqBuffer, 4 + SHA_DIGEST_LENGTH, req2hash);

		for (int i = 0; i < 20; i++)
			dataPtr[i] ^= req2hash[i];

		dataPtr += SHA_DIGEST_LENGTH;
	}

	prepareEncryption(true);

	memset(dataPtr, 0, 8);
	dataPtr += 8;

	*reinterpret_cast<uint32_t*>(dataPtr) = swap32(3);
	dataPtr += 4;

	*reinterpret_cast<uint16_t*>(dataPtr) = swap16((uint16_t)pad);
	dataPtr += 2;

	Random::Data(dataPtr, pad);
	dataPtr += pad;

	*reinterpret_cast<uint16_t*>(dataPtr) = swap16((uint16_t)pe3BtMessage.size());
	dataPtr += 2;

	if (!pe3BtMessage.empty())
	{
		memcpy(dataPtr, pe3BtMessage.data(), pe3BtMessage.size());
	}

	pe->encrypt(buffer.data() + 2 * SHA_DIGEST_LENGTH, buffer.size() - 2 * SHA_DIGEST_LENGTH);
}

size_t ProtocolEncryptionHandshake::readPe3Handshake(const BufferView& data, ProtocolEncryptionListener& l)
{
	if (data.size < Pe3DataSizePrePadding + 2)
		return 0;

	size_t possibleStartMax = data.size - Pe3DataSizePrePadding - 2;

	uint8_t reqBuffer[4 + 96] = { 'r', 'e', 'q', '1' };
	sharedSecret.Export(reqBuffer + 4, 96);
	uint8_t reqSha[SHA_DIGEST_LENGTH];
	_SHA1(reqBuffer, sizeof(reqBuffer), reqSha);

	size_t pos = 0;
	for (; pos <= possibleStartMax; pos++)
	{
		if (memcmp(data.data + pos, reqSha, SHA_DIGEST_LENGTH) == 0)
			break;
	}

	if (pos > possibleStartMax)
		return 0;

	auto minimumReadSize = pos;

	pos += SHA_DIGEST_LENGTH;

	{
		reqBuffer[3] = '3';
		_SHA1(reqBuffer, sizeof(reqBuffer), reqSha);

		infoHash = l.findByPe3Hash(data.data + pos, reqBuffer);

		if (!infoHash)
		{
			nextStep = Step::Error;
			return data.size;
		}

		pos += SHA_DIGEST_LENGTH;
	}

	prepareEncryption(false);

	//dont edit source data in case we need to retry
	DataBuffer buffer;
	buffer.reserve(72); //maybe ia sz + bt handshake
	pe->decrypt(data.data + pos, 8 + 4 + 2, buffer);

	PacketReader reader(buffer);
	if (reader.pop64() != 0)
	{
		nextStep = Step::Error;
		return data.size;
	}

	auto options = reader.pop32();

	if (options & 2)
		type = Type::RC4;
	else if (options & 1)
		type = Type::PlainText;
	else
	{
		nextStep = Step::Error;
		return data.size;
	}

	auto pad = reader.pop16();

	pos += 8 + 4 + 2;
	if (pos + uint32_t(pad) + 2 > data.size)
		return minimumReadSize;

	pe->rcIn.skip(pad);
	pos += pad;
	pe->decrypt(data.data + pos, data.size - pos, buffer);

	reader = PacketReader(buffer);
	auto iaSize = reader.pop16();

	if (reader.getRemainingSize() < iaSize)
		return minimumReadSize;

	pe3BtMessage = reader.popBuffer(iaSize);

	if (reader.getRemainingSize() == 0)
		nextStep = Step::Established;
	else
		nextStep = Step::Error;

	return data.size;
}

constexpr size_t Pe4DataSizePrePadding = 8 + 4 + 2;

void ProtocolEncryptionHandshake::createPe4Handshake(DataBuffer& buffer)
{
	uint32_t pad = Random::Number() % 512;

	buffer.resize(Pe4DataSizePrePadding + pad);
	auto dataPtr = buffer.data();

	memset(dataPtr, 0, 8);
	dataPtr += 8;

	*reinterpret_cast<uint32_t*>(dataPtr) = swap32(type == Type::RC4 ? 2 : 1);
	dataPtr += 4;

	*reinterpret_cast<uint16_t*>(dataPtr) = swap16((uint16_t)pad);
	dataPtr += 2;

	Random::Data(dataPtr, pad);

	pe->encrypt(buffer.data(), buffer.size());
}

// ENCRYPT(VC, crypto_select, len(padD), padD), ENCRYPT2(Payload Stream)
size_t ProtocolEncryptionHandshake::readPe4Handshake(const BufferView& data)
{
	if (data.size < Pe4DataSizePrePadding)
		return 0;

	size_t possibleStartMax = data.size - Pe4DataSizePrePadding;

	uint8_t rcVC[8] = { 0 };
	pe->rcIn.decode(rcVC, 8);

	size_t pos = 0;
	for (; pos <= possibleStartMax; pos++)
	{
		if (memcmp(data.data + pos, rcVC, 8) == 0)
			break;
	}

	if (pos > possibleStartMax)
		return 0;

	auto minimumReadSize = pos;

	pos += 8;

	DataBuffer buffer;
	pe->decrypt(data.data + pos, 6, buffer);

	PacketReader reader(buffer);
	auto options = reader.pop32();

	if (options & 2)
		type = Type::RC4;
	else if (options & 1)
		type = Type::PlainText;
	else
	{
		nextStep = Step::Error;
		return data.size;
	}

	auto pad = reader.pop16();

	pos += 6 + pad;

	if (pos > data.size)
		return minimumReadSize;

	pe->rcIn.skip(pad);
	nextStep = Step::Established;

	return pos;
}

void ProtocolEncryptionHandshake::prepareEncryption(bool invoker)
{
	pe = std::make_unique<ProtocolEncryption>();

	uint8_t kData[4 + 96 + SHA_DIGEST_LENGTH] = { 'k', 'e', 'y', uint8_t(invoker ? 'A' : 'B') };

	sharedSecret.Export(kData + 4, 96);
	memcpy(kData + 100, infoHash, SHA_DIGEST_LENGTH);

	uint8_t kDataSha[20];

	_SHA1(kData, sizeof(kData), kDataSha);
	pe->rcOut.init(kDataSha, 20);

	kData[3] = uint8_t(invoker ? 'B' : 'A');

	_SHA1(kData, sizeof(kData), kDataSha);
	pe->rcIn.init(kDataSha, 20);

	//skip first 1024 bytes in rc4
	pe->rcIn.skip(1024);
	pe->rcOut.skip(1024);
}

#pragma once
#include <stdint.h>
#include <vector>
#include <sstream>

#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#define swap64 _byteswap_uint64

struct PacketReader
{
	uint8_t pop()
	{
		uint8_t i = *reinterpret_cast<const uint8_t*>(buffer + pos);
		pos++;
		return i;
	}

	uint16_t pop16()
	{
		uint16_t i = swap16(*reinterpret_cast<const uint16_t*>(buffer + pos));
		pos += sizeof i;
		return i;
	}

	uint32_t pop32()
	{
		uint32_t i = swap32(*reinterpret_cast<const uint32_t*>(buffer + pos));
		pos += sizeof i;
		return i;
	}

	uint64_t pop64()
	{
		uint64_t i = swap64(*reinterpret_cast<const uint64_t*>(buffer + pos));
		pos += sizeof i;
		return i;
	}

	DataBuffer popBuffer(size_t size)
	{
		auto out = DataBuffer(buffer + pos, buffer + pos + size);
		pos += size;
		return out;
	}

	PacketReader(DataBuffer& buffer) : buffer(buffer.data()), size(buffer.size())
	{
	}

	PacketReader(const char* buf, size_t sz) : buffer(buf), size(sz)
	{
	}

	size_t getRemainingSize()
	{
		return size - pos;
	}

private:

	const char* buffer = nullptr;
	size_t size = 0;
	size_t pos = 0;
};

struct PacketBuilder
{
	void add(char c)
	{
		buf.sputn(&c, 1);
		size++;
	}

	void add16(uint16_t i)
	{
		i = swap16(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add32(uint32_t i)
	{
		i = swap32(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add64(uint64_t i)
	{
		i = swap64(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add(const char* b, size_t length)
	{
		buf.sputn(b, length);
		size += length;
	}

	DataBuffer getBuffer()
	{
		DataBuffer out;

		out.resize(size);
		buf.sgetn(&out[0], size);

		return out;
	}

	std::stringbuf buf;
	uint64_t size = 0;
};
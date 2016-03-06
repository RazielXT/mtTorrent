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
		uint8_t i = *reinterpret_cast<uint8_t*>(buf.data() + pos);
		pos++;
		return i;
	}

	uint16_t pop16()
	{
		uint16_t i = swap16(*reinterpret_cast<uint16_t*>(buf.data() + pos));
		pos += sizeof i;
		return i;
	}

	uint32_t pop32()
	{
		uint32_t i = swap32(*reinterpret_cast<uint32_t*>(buf.data() + pos));
		pos += sizeof i;
		return i;
	}

	uint64_t pop64()
	{
		uint64_t i = swap64(*reinterpret_cast<uint64_t*>(buf.data() + pos));
		pos += sizeof i;
		return i;
	}

	std::vector<char> popBuffer(size_t size)
	{
		auto out = std::vector<char>(buf.begin() + pos, buf.begin() + pos + size);
		pos += size;
		return out;
	}

	PacketReader(std::vector<char>& buffer) : buf(buffer)
	{
	}

	size_t getRemainingSize()
	{
		return buf.size() - pos;
	}

private:

	std::vector<char>& buf;
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

	std::vector<char> getBuffer()
	{
		std::vector<char> out;

		out.resize(size);
		buf.sgetn(&out[0], size);

		return out;
	}

	std::stringbuf buf;
	uint64_t size = 0;
};
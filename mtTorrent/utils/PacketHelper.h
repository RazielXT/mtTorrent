#pragma once

#include "ByteSwap.h"
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>

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

	BufferView popBufferView(size_t size)
	{
		auto out = BufferView{ buffer + pos, size };
		pos += size;
		return out;
	}

	PacketReader(const BufferView& buffer) : buffer(buffer.data), size(buffer.size)
	{
	}

	PacketReader(const void* buf, size_t sz) : buffer((const uint8_t*)buf), size(sz)
	{
	}

	size_t getRemainingSize()
	{
		return size - pos;
	}

	const uint8_t* popRaw(size_t size)
	{
		auto ptr = buffer + pos;
		pos += size;
		return ptr;
	}

	void move(size_t size)
	{
		pos += size;
	}

private:

	const uint8_t* buffer = nullptr;
	size_t size = 0;
	size_t pos = 0;
};

struct PacketBuilder
{
	PacketBuilder() = default;

	PacketBuilder(size_t size)
	{
		data.reserve(size);
	}

	void add(char c)
	{
		data.push_back(c);
	}

	void add16(uint16_t i)
	{
		i = swap16(i);
		auto ci = reinterpret_cast<char*>(&i);
		data.insert(data.end(), ci, ci + sizeof i);
	}

	static void Assign32(uint8_t* data, uint32_t i)
	{
		*reinterpret_cast<uint32_t*>(data) = swap32(i);
	}

	static void Append32(DataBuffer& data, uint32_t i)
	{
		i = swap32(i);
		auto ci = reinterpret_cast<char*>(&i);
		data.insert(data.end(), ci, ci + sizeof i);
	}

	void add32(uint32_t i)
	{
		Append32(data, i);
	}

	void add64(uint64_t i)
	{
		i = swap64(i);
		auto ci = reinterpret_cast<char*>(&i);
		data.insert(data.end(), ci, ci + sizeof i);
	}

	void addString(const char* b)
	{
		size_t length = strlen(b);
		data.insert(data.end(), b, b + length);
	}

	void add(const char* b, size_t length)
	{
		data.insert(data.end(), b, b + length);
	}

	PacketBuilder& operator<<(const char* rhs)
	{
		addString(rhs);
		return *this;
	}

	PacketBuilder& operator<<(const std::string& rhs)
	{
		add(rhs.data(), rhs.length());
		return *this;
	}

	template<typename T>
	void add(const T* b, size_t length)
	{
		data.insert(data.end(), b, b + length);
	}

	void addAfter(const char* find, const char* b, size_t length)
	{
		auto findLen = strlen(find);
		auto f = std::find_if(data.begin(), data.end() - length, [=](uint8_t& idx) { return memcmp(&idx, find, findLen) == 0; });

		if (f != data.end())
			f += findLen;

		data.insert(f, b, b + length);
	}

	void insert(size_t pos, const char* b, size_t length)
	{
		data.insert(data.begin() + pos, b, b + length);
	}

	DataBuffer& getBuffer()
	{
		return data;
	}

	DataBuffer data;
};
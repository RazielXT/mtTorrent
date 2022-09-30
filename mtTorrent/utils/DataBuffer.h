#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

using DataBuffer = std::vector<uint8_t>;

namespace Random
{
	void Data(DataBuffer&, std::size_t offset = 0);
	void Data(uint8_t*, std::size_t size);
	uint32_t Number();
	uint64_t Number64();
}

struct BufferSpan
{
	BufferSpan(DataBuffer&);
	BufferSpan(uint8_t* data, std::size_t size);

	uint8_t* data = nullptr;
	size_t size = 0;

	inline BufferSpan getOffset(size_t i) { return { data + i, size - i }; }
};

struct BufferView
{
	BufferView();
	BufferView(const BufferSpan&);
	BufferView(const DataBuffer&);
	BufferView(DataBuffer&&);
	BufferView(const uint8_t* data, std::size_t size);

	const uint8_t* data = nullptr;
	size_t size = 0;

	void store(const uint8_t* data, std::size_t size);
	void store();
private:
	DataBuffer localbuffer;
};

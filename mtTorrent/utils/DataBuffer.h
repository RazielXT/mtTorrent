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

struct BufferView
{
	BufferView() = default;
	BufferView(const DataBuffer&);
	BufferView(const void* data, std::size_t size);

	const uint8_t* data = nullptr;
	size_t size = 0;

	inline BufferView getOffset(size_t i) { return { data + i, size - i }; }
};

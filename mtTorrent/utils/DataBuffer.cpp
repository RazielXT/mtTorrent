#include "DataBuffer.h"
#include <random>
#include <cstring>

BufferView::BufferView(const void* d, std::size_t s)
{
	data = (const uint8_t*)d;
	size = s;
}

BufferView::BufferView(const DataBuffer& d)
{
	data = d.data();
	size = d.size();
}

uint32_t Random::Number()
{
	thread_local static std::mt19937 rng((int)time(nullptr));
	return rng();
}

std::mt19937_64& RandomNumber64Gen()
{
	thread_local static std::mt19937_64 rng((int)time(nullptr));
	return rng;
}

uint64_t Random::Number64()
{
	return RandomNumber64Gen()();
}

void Random::Data(DataBuffer& data, std::size_t offset /*= 0*/)
{
	return Random::Data(data.data() + offset, data.size() - offset);
}

void Random::Data(uint8_t* data, std::size_t size)
{
	auto ptr = (uint64_t*)data;
	auto bytesTail = size % sizeof(uint64_t);

	auto& rnd = RandomNumber64Gen();

	for (size_t i = 0; i < size - bytesTail; i += sizeof(uint64_t))
		*ptr++ = rnd();

	if (bytesTail)
	{
		auto rndData = rnd();
		memcpy(ptr, &rndData, bytesTail);
	}
}

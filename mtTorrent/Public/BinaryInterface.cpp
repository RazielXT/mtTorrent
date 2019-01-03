#include "BinaryInterface.h"


void* mtBI::string::basic_allocator::alloc(size_t size)
{
	return malloc(size);
}
void mtBI::string::basic_allocator::dealloc(void* data)
{
	free(data);
}

static mtBI::string::basic_allocator sallocator;

mtBI::string::string()
{
	allocator = &sallocator;

	data = (char*)allocator->alloc(100);
	data[0] = 0;
}

mtBI::string::~string()
{
	if (data)
		sallocator.dealloc(data);
}

void mtBI::string::set(const std::string& str)
{
	//if (data)
	//	sallocator.dealloc(data);

	//data = (char*)allocator->alloc(str.length() + 1);
	memcpy(data, str.data(), str.length() + 1);
}

void mtBI::string::add(const std::string& str)
{
	//if (data)
	//	sallocator.dealloc(data);

	//data = (char*)allocator->alloc(str.length() + 1);

	memcpy(data + strlen((char*)data), str.data(), str.length() + 1);
}

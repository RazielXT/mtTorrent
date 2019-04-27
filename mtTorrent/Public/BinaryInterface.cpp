#include "BinaryInterface.h"
#include "ModuleAllocator.h"

mtBI::string::string() : allocator(&mtBI::module_allocator<>::m_allocator)
{
}

mtBI::string::~string()
{
	if (data)
		allocator->deallocate(data);
}

void mtBI::string::assign(const char* str, size_t length)
{
	if (data)
		allocator->deallocate(data);

	data = (char*)allocator->allocate(length + 1);
	memcpy(data, str, length + 1);
}

mtBI::string& mtBI::string::operator=(const char* str)
{
	assign(str, strlen(str));

	return *this;
}

mtBI::string& mtBI::string::operator=(const std::string& str)
{
	assign(str.data(), str.length());

	return *this;
}

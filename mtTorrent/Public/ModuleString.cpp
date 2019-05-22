#include "ModuleString.h"
#include "ModuleAllocator.h"

mtt::string::string() : allocator(&mtt::module_allocator<>::m_allocator)
{
}

mtt::string::~string()
{
	if (data)
		allocator->deallocate(data);
}

void mtt::string::assign(const char* str, size_t l)
{
	if (data)
		allocator->deallocate(data);

	length = l;
	data = (char*)allocator->allocate(length + 1);
	memcpy(data, str, length + 1);
}

mtt::string & mtt::string::operator=(const char* str)
{
	assign(str, strlen(str));

	return *this;
}

mtt::string& mtt::string::operator=(const std::string & str)
{
	assign(str.data(), str.length());

	return *this;
}

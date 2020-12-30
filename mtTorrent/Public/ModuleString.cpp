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

	if (str)
	{
		data = (char*)allocator->allocate(length + 1);
		memcpy(data, str, length + 1);
	}
	else
		data = nullptr;
}

void mtt::string::append(const string& str)
{
	auto newData = (char*)allocator->allocate(length + str.length + 1);

	if (data)
	{
		memcpy(newData, data, length);
		allocator->deallocate(data);
	}

	data = newData;
	memcpy(data + length, str.data, str.length + 1);
	length += str.length;
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

mtt::string& mtt::string::operator=(const string& str)
{
	assign(str.data, str.length);

	return *this;
}

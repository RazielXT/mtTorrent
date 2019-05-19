#pragma once

namespace mtt
{
	typedef void* (__cdecl allocate_type)(size_t size);
	typedef void(__cdecl deallocate_type)(void* ptr);

	inline void* __cdecl allocate_impl(size_t size)
	{
		return ::operator new(size);
	}

	inline void __cdecl deallocate_impl(void* ptr)
	{
		::operator delete(ptr);
	}

	/*
	Encapsulates malloc & free. There is exactly 1 allocator instance per module.
	*/
#pragma pack(push, 4)
	class allocator
	{
	public:

		// points to allocate_impl
		allocate_type* m_allocate_ptr;
		// points to deallocate_impl
		deallocate_type* m_deallocate_ptr;

		void* allocate(size_t size) const
		{
			return m_allocate_ptr(size);
		}
		void deallocate(void* ptr) const
		{
			return m_deallocate_ptr(ptr);
		}
	};
#pragma pack(pop)

	/*
	Common allocator for this module. We use template so we don't have to define the variable in a cpp file.
	*/
	template <class Dummy = void> struct module_allocator
	{
		static const allocator m_allocator;
	};

	template <class Dummy> const allocator module_allocator<Dummy>::m_allocator =
	{
		allocate_impl,
		deallocate_impl
	};
};

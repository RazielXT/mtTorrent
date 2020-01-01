#pragma once
#include "ModuleAllocator.h"

namespace mtt
{
	template <class T>
	struct array
	{
		array() : allocator(&mtt::module_allocator<>::m_allocator)
		{
		}

		~array()
		{
			clear();
		}

		void reserve(size_t size)
		{
			auto wantedSize = size * sizeof(T);
			if (wantedSize > allocatedSize)
			{
				auto old = data;
				data = (T*)allocator->allocate(wantedSize);

				if (old)
				{
					memcpy(data, old, allocatedSize);
					allocator->deallocate(old);
				}

				allocatedSize = wantedSize;
			}
		}

		void resize(size_t size)
		{
			reserve(size);
			this->size = size;
		}

		T& operator [](size_t idx)
		{
			return data[idx];
		}

		void add(T& item)
		{
			reserve(size + 1);
			data[size] = item;
			size++;
		}

		size_t count()
		{
			return size;
		}

		void clear()
		{
			for(size_t i = 0; i < size; i++)
				data[i].~T();

			if (data)
				allocator->deallocate(data);

			data = nullptr;
			size = 0;
			allocatedSize = 0;
		}

	private:
		const allocator* const allocator;

		T* data = nullptr;
		size_t size = 0;
		size_t allocatedSize = 0;
	};
}
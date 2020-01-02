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
				auto old = buffer;
				buffer = (T*)allocator->allocate(wantedSize);

				if (old)
				{
					memcpy(buffer, old, allocatedSize);
					allocator->deallocate(old);
				}

				allocatedSize = wantedSize;
			}
		}

		void resize(size_t size)
		{
			if (size > currentSize)
			{
				reserve(size);

				for (size_t i = 0; i < size; i++)
				{
					new (&buffer[i]) T();
				}
			}

			this->currentSize = size;
		}

		void assign(const T* items, size_t size)
		{
			resize(size);

			if(size > 0)
				memcpy(buffer, items, size * sizeof(T));
		}

		void assign(const T value, size_t count)
		{
			resize(count);

			for (size_t i = 0; i < count; i++)
			{
				buffer[i] = value;
			}
		}

		T& operator [](size_t idx)
		{
			return buffer[idx];
		}

		void add(T& item)
		{
			reserve(currentSize + 1);
			buffer[currentSize] = item;
			currentSize++;
		}

		size_t size()
		{
			return currentSize;
		}

		bool empty()
		{
			return currentSize == 0;
		}

		T* data()
		{
			return buffer;
		}

		void clear()
		{
			for(size_t i = 0; i < currentSize; i++)
				buffer[i].~T();

			if (buffer)
				allocator->deallocate(buffer);

			buffer = nullptr;
			currentSize = 0;
			allocatedSize = 0;
		}

		struct iterator {
		public:
			iterator(const T* ptr) : ptr(ptr) {}
			iterator operator++() { ptr++; return *this; }
			bool operator!=(const iterator& other) const { return ptr != other.ptr; }
			const T& operator*() const { return *ptr; }
			T& operator*() { return *const_cast<T*>(ptr); }
		private:
			const T* ptr;
		};

		iterator begin() const { return iterator(buffer); };
		iterator end() const { return iterator(buffer + currentSize); };

		// Move constructor
		array(array&& other) noexcept : allocator(other.allocator)
		{
			buffer = other.buffer;
			currentSize = other.currentSize;
			allocatedSize = other.allocatedSize;
			other.buffer = nullptr;
			other.currentSize = 0;
			other.allocatedSize = 0;
		}

		array& operator=(array&& rhs) noexcept {

			buffer = rhs.buffer;
			currentSize = rhs.currentSize;
			allocatedSize = rhs.allocatedSize;
			rhs.buffer = nullptr;
			rhs.currentSize = 0;
			rhs.allocatedSize = 0;

			return *this;
		}

	private:
		const allocator* const allocator;

		T* buffer = nullptr;
		size_t currentSize = 0;
		size_t allocatedSize = 0;
	};
}
#pragma once

#include <cstdlib>
#include <cassert>


template <typename T>
class MemoryBuffer
{
public:
	T* data = nullptr;

	size_t capacity_ = 0;
	size_t size_ = 0;
};


template <typename T>
class MemoryView
{
public:
	T* begin = nullptr;
	size_t length;
};


namespace memory_buffer
{
	template <typename T>
	bool create_buffer(MemoryBuffer<T>& buffer, size_t n_elements)
	{
		assert(n_elements > 0);
		assert(!buffer.data);

		if (n_elements == 0 || buffer.data)
		{
			return false;
		}

		buffer.data = (T*)std::malloc(n_elements * sizeof(T));		
		assert(buffer.data);

		if (!buffer.data)
		{
			return false;
		}

		buffer.capacity_ = n_elements;
		buffer.size_ = 0;

		return true;
	}


	template <typename T>
	void destroy_buffer(MemoryBuffer<T>& buffer)
	{
		if (buffer.data)
		{
			std::free(buffer.data);
		}		

		buffer.data = nullptr;
		buffer.capacity_ = 0;
		buffer.size_ = 0;
	}

	template <typename T>
	void reset_buffer(MemoryBuffer<T>& buffer)
	{
		buffer.size_ = 0;
	}


	template <typename T>
	T* push_elements(MemoryBuffer<T>& buffer, size_t n_elements)
	{
		assert(n_elements > 0);

		if (n_elements == 0)
		{
			return nullptr;
		}

		assert(buffer.data);
		assert(buffer.capacity_);

		auto is_valid =
			buffer.data &&
			buffer.capacity_ &&
			buffer.size_ < buffer.capacity_;

		auto elements_available = (buffer.capacity_ - buffer.size_) >= n_elements;
		assert(elements_available);

		if (!is_valid || !elements_available)
		{
			return nullptr;
		}

		auto data = buffer.data + buffer.size_;

		buffer.size_ += n_elements;

		return data;
	}


	template <typename T>
	void pop_elements(MemoryBuffer<T>& buffer, size_t n_elements)
	{
		if (!n_elements)
		{
			return;
		}

		assert(buffer.data);
		assert(buffer.capacity_);
		assert(n_elements <= buffer.size_);

		if(n_elements > buffer.size_)
		{
			buffer.size_ = 0;
		}
		else
		{
			buffer.size_ -= n_elements;
		}
	}


	template <typename T>
	bool create_buffer(MemoryBuffer<T>& buffer, unsigned n_elements)
	{
		return create_buffer(buffer, (size_t)n_elements);
	}


	template <typename T>
	T* push_elements(MemoryBuffer<T>& buffer, unsigned n_elements)
	{
		return push_elements(buffer, (size_t)n_elements);
	}


	template <typename T>
	void pop_elements(MemoryBuffer<T>& buffer, unsigned n_elements)
	{
		pop_elements(buffer, (size_t)n_elements);
	}


	template <typename T>
	MemoryView<T> push_view(MemoryBuffer<T>& buffer, size_t n_elements)
	{
		assert(n_elements > 0);
		assert(buffer.data);
		assert(buffer.capacity_);

		auto elements_available = (buffer.capacity_ - buffer.size_) >= n_elements;
		assert(elements_available);

		MemoryView<T> view{};

		view.begin = push_elements(buffer, n_elements);
		view.length = n_elements;

		return view;
	}


	template <typename T>
	MemoryView<T> make_view(MemoryBuffer<T>& buffer)
	{
		assert(buffer.data);
		assert(buffer.size_);
		assert(buffer.capacity_);

		MemoryView<T> view{};

		view.begin = buffer.data;
		view.length = buffer.size_;

		return view;
	}
}

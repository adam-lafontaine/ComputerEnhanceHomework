#pragma once

#include <filesystem>
#include "memory_buffer.hpp"

namespace mb = memory_buffer;
namespace fs = std::filesystem;


namespace memory_buffer
{
	template <typename T>
	MemoryBuffer<T> read_buffer(fs::path const& path)
	{
		MemoryBuffer<T> buffer{};

		auto size = fs::file_size(path);
		auto n_elements = size / sizeof(T);

		if (size == 0 || !mb::create_buffer(buffer, n_elements))
		{
			assert(false);
			return buffer;
		}
		
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			assert(false);
			mb::destroy_buffer(buffer);
			return buffer;
		}

		file.read((char*)buffer.data, size);
		buffer.size_ = n_elements;

		file.close();

		return buffer;
	}
}
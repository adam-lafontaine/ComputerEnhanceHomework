#include <cmath>
#include <fstream>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"

#include "memory_buffer.hpp"

namespace mb = memory_buffer;


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


#include "json_write.cpp"
#include "json_read.cpp"
#include "bin_read.cpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}


void print_directory(cstr dir)
{
    for (auto const& entry : fs::directory_iterator(dir))
    {
        std::cout << entry.path() << '\n';
    }
}


static void print(HavOut const& result)
{
    if (result.error)
    {
        printf("Error: %s", result.msg);
    }
    else
    {
        printf("   Input size: %lu\n", result.input_size);
        printf("   Pair count: %u\n", result.input_count);
        printf("Haversine avg: %lf\n", result.avg);
    }
}


void print_results(HavOut const& result, HavOut const& ref)
{
    printf("JSON:\n");
    print(result);

    printf("\n");

    printf("Validation:\n");
    print(ref);
    printf("Difference: %lf\n", (result.avg - ref.avg));
}
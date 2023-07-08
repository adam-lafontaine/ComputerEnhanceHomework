#include <cmath>
#include <fstream>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

#include "lib.hpp"
#include "memory_buffer.hpp"
#include "profiler.cpp"

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

#include "bin_read.cpp"

#include "listing_0065_haversine_formula.cpp"
#include "json_write.cpp"
#include "json_read.cpp"



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


void print(HavOut const& result)
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


#include "listing_0070_platform_metrics.cpp"
#include "perf.cpp"


void print(HavProf const& prof)
{
	auto const pct = [&](u64 n){ return (f64)n / prof.cpu_total * 100; };

	printf("Total time: %lf ms (CPU freq %lu)\n", prof.total_ms, prof.cpu_freq);
	printf("  Startup: %lu (%2.2f%%)\n", prof.cpu_startup, pct(prof.cpu_startup));
	printf("     Read: %lu (%2.2f%%)\n", prof.cpu_read, pct(prof.cpu_read));
	printf("    Setup: %lu (%2.2f%%)\n", prof.cpu_setup, pct(prof.cpu_setup));
	printf("  Process: %lu (%2.2f%%)\n", prof.cpu_process, pct(prof.cpu_process));
	printf("  Cleanup: %lu (%2.2f%%)\n", prof.cpu_cleanup, pct(prof.cpu_cleanup));
}
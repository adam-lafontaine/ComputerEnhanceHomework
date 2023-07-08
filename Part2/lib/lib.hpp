#pragma once

#include "types.hpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1);

void haversine_json(cstr out_dir, u32 n_pairs);

HavOut process_json(cstr json_path);

HavOut process_bin(cstr bin_path);

void print_directory(cstr dir);

void print(HavOut const& result);

void print_results(HavOut const& result, HavOut const& ref);

HavOut process_json(cstr json_path, HavProf& prof);

void print(HavProf const& prof);


namespace perf
{
    u64 os_read_ticks();

    u64 cpu_read_ticks();

    f64 est_os_ms(u64 os_ticks);

    f64 est_cpu_ms(u64 cpu_ticks);

    u64 est_cpu_freq(u64 cpu_ticks, u64 os_ticks);

    u64 est_cpu_freq();
}

#include "profiler.hpp"
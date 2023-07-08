#include "profiler.hpp"

#include <cstdio>
#include <algorithm>


namespace
{
    class ProfileRecord
    {
    public:

        u64 cpu_total = 0;

        u32 hit_count = 0;

        b32 is_active = false;
    };
}


constexpr auto N_PROFILE_RECORDS = (int)perf::ProfileLabel::Count;


static ProfileRecord g_records[N_PROFILE_RECORDS] = { 0 };


namespace perf
{
    Profile::Profile(ProfileLabel label)
    {
        profile_id = (int)label;

        auto& record = g_records[profile_id];

        if (record.is_active)
        {
            // recursive
            profile_id = -1;
        }
        else
        {
            record.is_active = true;
            cpu_start = cpu_read_ticks();
        }
    }


    Profile::~Profile()
    {
        if (profile_id == -1)
        {
            return;
        }

        cpu_end = cpu_read_ticks();
        auto& record = g_records[profile_id];

        record.cpu_total += cpu_end - cpu_start;
        ++record.hit_count;
        record.is_active = false;
    }
}




namespace perf
{
    using PL = perf::ProfileLabel;


    void profile_init()
    {
        for (u32 i = 0; i < (u32)PL::Count; ++i)
        {
            g_records[i] = { 0 };
        }
    }


    void profile_clear()
    {
        profile_init();
    }


    void profile_report()
    {
        auto begin = g_records;
        auto end = g_records + N_PROFILE_RECORDS;
        auto min = std::min_element(begin, end, [](auto lhs, auto rhs){ return lhs.cpu_total < rhs.cpu_total; });

        printf("Profile Report:\n");

        for (u32 i = 0; i < (u32)PL::Count; ++i)
        {
            auto& record = g_records[i];

            auto label = to_cstr((PL)i);
            auto cpu_abs = record.cpu_total;
            auto cpu_rel = cpu_abs / min->cpu_total;

            printf("%s: %lu (%lu)\n", label, cpu_rel, cpu_abs);
        }
    }
}
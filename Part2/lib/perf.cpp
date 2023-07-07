namespace perf
{
    static u64 CPU_HZ = 0;


    u64 os_read_ticks()
    {
        return ReadOSTimer();
    }


    u64 cpu_read_ticks()
    {
        return ReadCPUTimer();
    }


    u64 est_cpu_freq(u64 cpu_ticks, u64 os_ticks)
    {
        return GetOSTimerFreq() * cpu_ticks / os_ticks;
    }


    u64 est_cpu_freq()
    {
        u64 wait_ms = 1000;
        auto os_freq = GetOSTimerFreq();
        auto os_wait = os_freq * wait_ms / 1000;
        
        auto os_start = os_read_ticks();
        auto cpu_start = cpu_read_ticks();
        auto os_end = os_read_ticks();

        auto os_ticks = os_end - os_start;

        while (os_ticks < os_wait)
        {
            os_end = os_read_ticks();
            os_ticks = os_end - os_start;
        }

        auto cpu_end = cpu_read_ticks();

        perf::CPU_HZ = os_freq * (cpu_end - cpu_start) / os_ticks;

        return perf::CPU_HZ;
    }


    f64 est_os_ms(u64 os_ticks)
    {
        return 1000.0 * os_ticks / GetOSTimerFreq();
    }


    f64 est_cpu_ms(u64 cpu_ticks)
    {
        if (!perf::CPU_HZ)
        {
            return 1000.0 * cpu_ticks / est_cpu_freq();
        }

        return 1000.0 * cpu_ticks / perf::CPU_HZ;
    }
}
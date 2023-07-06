namespace perf
{
    u64 os_ticks()
    {
        return ReadOSTimer();
    }


    u64 cpu_ticks()
    {
        return ReadCPUTimer();
    }


    u64 est_cpu_freq(u64 cpu_ticks, u64 os_ticks)
    {
        return GetOSTimerFreq() * cpu_ticks / os_ticks;
    }


    f64 est_ms(u64 os_ticks)
    {
        return 1000.0 * os_ticks / GetOSTimerFreq();
    }
}
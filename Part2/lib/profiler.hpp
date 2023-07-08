

namespace perf
{
    enum class ProfileLabel : int
    {
        Read,
        Process,
        Cleanup,

        Count,
        None = -1
    };


    inline cstr to_cstr(ProfileLabel label)
    {
        using PL = perf::ProfileLabel;

        switch(label)
        {
            case PL::Read: return "Read";
            case PL::Process: return "Process";
            case PL::Cleanup: return "Cleanup";
        }

        return "err";
    }
}


namespace perf
{
    class Profile
    {
    public:
        
        Profile(ProfileLabel label);

        ~Profile();

        int profile_id = 0;

    
    private:

        u64 cpu_start;
        u64 cpu_end;
    };
}


namespace perf
{
    void profile_init();

    void profile_clear();

    void profile_report();
}


namespace perf
{
    enum class ProfileLabel : int
    {
        f1,
        f2,

        Count,
        None = -1
    };


    inline cstr to_cstr(ProfileLabel label)
    {
        using PL = perf::ProfileLabel;

        switch(label)
        {
            case PL::f1: return "TODO";
            case PL::f2: return "TODO";
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
#include "../lib/lib.hpp"

#include <cassert>


int main()
{
    if(!fs::exists("out"))
    {
        auto res = fs::create_directories("out");
        assert(res);
    }
    

    haversine_json("out/pairs.json", 10);
}
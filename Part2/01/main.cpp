#include "../lib/lib.hpp"

#include <cassert>


int main()
{
    auto res = fs::create_directories("out");
    assert(res);

    haversine_json("out/pairs.json");
}
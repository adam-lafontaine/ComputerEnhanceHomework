#include <cmath>
#include <iostream>
#include <fstream>
#include <random>

#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"


using Dist = std::normal_distribution<f64>;


constexpr f64 X_MIN = -180.0;
constexpr f64 X_MAX = 180.0;
constexpr f64 Y_MIN = -90.0;
constexpr f64 Y_MAX = 90.0;


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}


static void write_pair(std::ofstream& out, f64 val)
{
    f64 x0 = val;
    f64 y0 = val;
    f64 x1 = val;
    f64 x2 = val;

    out << "{\"X0\":" << x0 << ",\"Y0\":" << y0;
    out << ",\"X1\":" << x1 << ",\"Y1\":" << y0 << "}";
}


void haversine_json(fs::path const& json_path, u32 n_pairs)
{
    //std::random_device rd {};
    //std::mt19937 gen {rd()};

    std::ofstream out(json_path, std::ios::out);

    f64 x0 = 0.0;
    f64 y0 = 0.0;
    f64 x1 = 0.0;
    f64 x2 = 0.0;

    out << "{\"pairs\":[";

    for (u32 i = 0; i < n_pairs - 1; ++i)
    {
        write_pair(out, i);
        out << ",";
    }

    write_pair(out, 99);

    out << "]}";
    out.close();
}
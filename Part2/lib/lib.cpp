#include <cmath>
#include <iostream>
#include <fstream>
#include <random>

#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"

using RD = std::random_device;
using Gen = std::mt19937;
using Dist = std::normal_distribution<f64>;


constexpr f64 X_MIN = -180.0;
constexpr f64 X_MAX = 180.0;
constexpr f64 Y_MIN = -90.0;
constexpr f64 Y_MAX = 90.0;


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}


static f64 scale_x(f64 val)
{
    return X_MIN + val * (X_MAX - X_MIN);
}


static f64 scale_y(f64 val)
{
    return Y_MIN + val * (Y_MAX - Y_MIN);
}


static void write_pair(std::ofstream& out, Dist& dist, Gen& gen)
{
    f64 x0 = scale_x(dist(gen));
    f64 y0 = scale_y(dist(gen));
    f64 x1 = scale_x(dist(gen));
    f64 y1 = scale_y(dist(gen));

    out << "{\"X0\":" << x0 << ",\"Y0\":" << y0;
    out << ",\"X1\":" << x1 << ",\"Y1\":" << y1 << "}";
}


void haversine_json(fs::path const& json_path, u32 n_pairs)
{
    RD rd {};
    Gen gen {rd()};

    f64 mean = 0.5;
    f64 sd = 0.2;

    Dist dist { mean, sd };

    std::ofstream out(json_path, std::ios::out);

    f64 x0 = 0.0;
    f64 y0 = 0.0;
    f64 x1 = 0.0;
    f64 x2 = 0.0;

    out << "{\"pairs\":[";

    for (u32 i = 0; i < n_pairs - 1; ++i)
    {
        write_pair(out, dist, gen);
        out << ",";
    }

    write_pair(out, dist, gen);

    out << "]}";
    out.close();
}
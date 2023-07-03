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


static f64 wrap(f64 val)
{
    f64 i = 0.0;
    f64 f = std::modf(val, &i);

    if (f < 0.0)
    {
        f += 1.0;
    }

    return f;
}


static f64 scale_x(f64 val)
{
    val = wrap(val);
    return X_MIN + val * (X_MAX - X_MIN);
}


static f64 scale_y(f64 val)
{
    val = wrap(val);
    return Y_MIN + val * (Y_MAX - Y_MIN);
}


static void write_pair(std::ofstream& out, Gen& gen, Dist& dist_x, Dist& dist_y)
{
    f64 x0 = scale_x(dist_x(gen));
    f64 y0 = scale_y(dist_y(gen));
    f64 x1 = scale_x(dist_x(gen));
    f64 y1 = scale_y(dist_y(gen));

    out << "{\"X0\":" << x0 << ",\"Y0\":" << y0;
    out << ",\"X1\":" << x1 << ",\"Y1\":" << y1 << "}";
    out << ",";
}


void haversine_json(fs::path const& json_path, u32 n_pairs)
{
    constexpr u32 n_clusters = 10;

    f64 x_means[10] = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 };
    f64 y_means[10] = { 0.5, 0.6, 0.7, 0.8, 0.9, 0.0, 0.1, 0.2, 0.3, 0.4 };

    RD rd {};
    Gen gen {rd()};
    
    f64 sd = 0.2;

    

    std::ofstream out(json_path, std::ios::out);

    f64 x0 = 0.0;
    f64 y0 = 0.0;
    f64 x1 = 0.0;
    f64 x2 = 0.0;

    out << "{\"pairs\":[";

    for (u32 c = 0; c < n_clusters; ++c)
    {
        Dist dist_x { x_means[c], sd };
        Dist dist_y { y_means[c], sd };

        for (u32 i = 0; i < n_pairs / n_clusters - 1; ++i)
        {
            write_pair(out, gen, dist_x, dist_y);
        }
    }    

    out.seekp(-1, std::ios_base::end);

    out << "]}";
    out.close();
}
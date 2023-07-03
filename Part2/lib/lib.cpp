#include <cmath>
#include <iostream>
#include <fstream>


#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}


void haversine_json(fs::path const& json_path)
{
    std::ofstream out(json_path, std::ios::out);

    out << "{\"pairs\":[]}";


    out.close();
}
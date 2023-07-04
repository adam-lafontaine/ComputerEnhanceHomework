#include <cmath>
#include <fstream>
#include <cassert>

#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"
#include "json_write.cpp"
#include "json_read.cpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}
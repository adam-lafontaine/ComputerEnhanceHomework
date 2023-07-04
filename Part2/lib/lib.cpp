#include "lib.hpp"
#include "listing_0065_haversine_formula.cpp"
#include "json_write.cpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1)
{
    return ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
}
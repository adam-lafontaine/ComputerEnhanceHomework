#pragma once

#include "types.hpp"


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1);

void haversine_json(cstr out_dir, u32 n_pairs);

HavOut process_json(cstr json_path);
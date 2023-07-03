#include "types.hpp"

#include <filesystem>

namespace fs = std::filesystem;


f64 haversine_earth(f64 X0, f64 Y0, f64 X1, f64 Y1);


void haversine_json(fs::path const& json_path, u32 n_pairs);
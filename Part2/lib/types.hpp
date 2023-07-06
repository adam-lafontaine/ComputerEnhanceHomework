#include <cstdint>

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using cstr = const char*;
using b32 = u32;


class HavOut
{
public:
    u64 input_size = 0;
    u32 input_count = 0;
    f64 avg = 0.0;

    b32 error = false;
    cstr msg = 0;
};
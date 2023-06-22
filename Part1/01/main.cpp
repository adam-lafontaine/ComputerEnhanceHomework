#include <fstream>
#include <filesystem>
#include <cassert>
#include <cstdlib>

namespace fs = std::filesystem;

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


class ByteBuffer
{
public:
    u32 size = 0;
    u8* data = nullptr;
};


static void destroy_buffer(ByteBuffer& buffer)
{
    if (buffer.data)
    {
        std::free(buffer.data);
    }
}


static bool create_buffer(ByteBuffer& buffer, u32 size)
{
    assert(!buffer.data);
    assert(size);
    
    if (buffer.data || !size)
    {
        return false;
    }

    buffer.data = (u8*)std::malloc(size);
    if (!buffer.data)
    {
        return false;
    }

    return true;
}


constexpr auto BIN_FILE = "listing_0037_single_register_mov";



static ByteBuffer read_bytes(fs::path const& path)
{
    ByteBuffer buffer;

    auto size = fs::file_size(path);
    if (size == 0 || !create_buffer(buffer, size))
    {
        assert(false);
        return buffer;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        assert(false);
        destroy_buffer(buffer);
        return buffer;
    }

    file.read((char*)buffer.data, size);
    buffer.size = size;

    file.close();
    return buffer;
}
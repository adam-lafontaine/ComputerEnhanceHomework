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
using cstr = const char*;


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


constexpr auto BIN_FILE = "listing_0038_many_register_mov";



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


static int parse_opcode(u8 byte)
{
    return (byte & 0b11111100) >> 2;
}


static int parse_direction(u8 byte)
{
    return (byte & 0b00000010) >> 1;
}


static int parse_word(u8 byte)
{
    return byte & 0b00000001;
}


static int parse_mode(u8 byte)
{
    return (byte & 0b11000000) >> 6;
}


static int parse_register(u8 byte)
{
    return (byte & 0b00111000) >> 3;
}


static int parse_register_memory(u8 byte)
{
    return byte & 0b00000111;
}


static cstr decode_opcode(int opcode)
{
    switch (opcode)
    {
        case 0b00100010: return "mov";
    }

    return  "opcode?";
}


static cstr decode_register(int reg, int word)
{
    if (word)
    {
        switch (reg)
        {
        case 0: return "ax";
        case 1: return "cx";
        case 2: return "dx";
        case 3: return "bx";
        case 4: return "sp";
        case 5: return "bp";
        case 6: return "si";
        case 7: return "di";
        }
    }
    else
    {
        switch (reg)
        {
        case 0: return "al";
        case 1: return "cl";
        case 2: return "dl";
        case 3: return "bl";
        case 4: return "ah";
        case 5: return "ch";
        case 6: return "dh";
        case 7: return "bh";
        }
    }

    return "register?";
}


static cstr decode_register_memory(int rm, int word, int mode)
{
    switch (mode)
    {
        case 0: return "mode0?";
        case 1: return "mode1?";
        case 2: return "mode2?";
        case 3: return decode_register(rm, word);
    }

    return "mode?";
}


static void decode_single(u8 byte1, u8 byte2)
{
    int opcode = parse_opcode(byte1);
    int direction = parse_direction(byte1);
    int word = parse_word(byte1);
    int mode = parse_mode(byte2);
    int reg = parse_register(byte2);
    int rm = parse_register_memory(byte2);

    auto op = decode_opcode(opcode);
    auto src = "src?";
    auto dst = "dst?";    

    if (direction)
    {
        src = decode_register_memory(rm, word, mode);
        dst = decode_register(reg, word);
    }
    else
    {
        src = decode_register(reg, word);
        dst = decode_register_memory(rm, word, mode);
    }

    printf("%s %s, %s\n", op, dst, src);
}


static void decode(cstr bin_file)
{
    auto buffer = read_bytes(bin_file);

    assert(buffer.data);
    assert(buffer.size);
    assert(buffer.size % 2 == 0);

    printf("bits 16\n\n");

    for(u32 i = 0; i < buffer.size; i += 2)
    {
        decode_single(buffer.data[i], buffer.data[i + 1]);
    }

    destroy_buffer(buffer);
}


int main()
{
    decode(BIN_FILE);
}
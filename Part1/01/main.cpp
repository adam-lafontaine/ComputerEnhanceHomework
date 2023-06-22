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
    return (byte & 0b11100000) >> 5;
}


static int parse_register(u8 byte)
{
    return (byte & 0b00111000) >> 3;
}


static int parse_register_memory(u8 byte)
{
    return byte & 0b00000111;
}


static char* decode_opcode(int opcode)
{
    switch (opcode)
    {
        case 0b00100010: return "mov";
    }

    return  "???";
}


static char* decode_register(int reg, int word)
{
    if (word)
    {
        switch (reg)
        {
        case 0: return "AX";
        case 1: return "CX";
        case 2: return "DX";
        case 3: return "BX";
        case 4: return "SP";
        case 5: return "BP";
        case 6: return "SI";
        case 7: return "DI";
        }
    }
    else
    {
        switch (reg)
        {
        case 0: return "AL";
        case 1: return "CL";
        case 2: return "DL";
        case 3: return "BL";
        case 4: return "AH";
        case 5: return "CH";
        case 6: return "DH";
        case 7: return "BH";
        }
    }

    return "???";
}


static char* decode_register_memory(int rm, int word, int mode)
{
    switch (mode)
    {
        case 0: return "mode?";
        case 1: return "mode?";
        case 2: return "mode?";
        case 3: return decode_register(rm, word);
    }
}


static void decode(const char* bin_file)
{
    auto buffer = read_bytes(bin_file);

    auto byte1 = buffer.data[0];
    auto byte2 = buffer.data[1];
    destroy_buffer(buffer);

    int opcode = parse_opcode(byte1);
    int direction = parse_direction(byte1);
    int word = parse_word(byte1);
    int mode = parse_mode(byte2);
    int reg = parse_register(byte2);
    int rm = parse_register_memory(byte2);


    std::ofstream out("out_0037.asm");
    if (!out.is_open())
    {
        return;
    }    

    char* op = decode_opcode(opcode);
    char* src = "src?";
    char* dst = "dst?";

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

    out << "bits 16\n\n";
    out << decode_opcode(opcode) << ' ' << dst << ", " << src << '\n';

    out.close();
}


int main()
{
    decode(BIN_FILE);
}
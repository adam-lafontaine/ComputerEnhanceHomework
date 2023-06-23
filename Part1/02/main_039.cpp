#include <fstream>
#include <filesystem>
#include <cassert>
#include <cstdlib>
#include <cstring>

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


enum class OpCode : int
{
    mov_rm_tf_r = 0b100010'00,
    mov_i_to_rm = 0b1100011'0,
    mov_i_to_r = 0b1011'0000,
    mov_m_to_a = 0b1010000'0,
    mov_a_to_m = 0b1010001'0,
    none = 0
};


enum class Dir : int
{
    to_r = 1,
    from_r = 0,
    none = -1
};


enum class Width : int
{
    byte = 0,
    word = 1,
    none = -1
};


enum class Mode : int
{
    mem = 0,
    mem_8 = 1,
    mem_16 = 2,
    reg = 3,
    none = -1
};


enum class Register : int
{
    // W = 0
    AL = 0b0'000,
    CL = 0b0'001,
    DL = 0b0'010,
    BL = 0b0'011,
    AH = 0b0'100,
    CH = 0b0'101,
    DH = 0b0'110,
    BH = 0b0'111,

    // W = 1
    AX = 0b1'000,
    CX = 0b1'001,
    DX = 0b1'010,
    BX = 0b1'011,
    SP = 0b1'100,
    BP = 0b1'101,
    SI = 0b1'110,
    DI = 0b1'111,

    none = -1
};


enum class RegisterMemory : int
{
    // = w + mod + rm
    mod_00_rm_000 = 0 + 0 + 0,
    mod_00_rm_001 = 0 + 0 + 1,
    mod_00_rm_010 = 0 + 0 + 2,
    mod_00_rm_011 = 0 + 0 + 3,
    mod_00_rm_100 = 0 + 0 + 4,
    mod_00_rm_101 = 0 + 0 + 5,
    mod_00_rm_110 = 0 + 0 + 6,
    mod_00_rm_111 = 0 + 0 + 7,

    mod_01_rm_000 = 0 + 8 + 0,
    mod_01_rm_001 = 0 + 8 + 1,
    mod_01_rm_010 = 0 + 8 + 2,
    mod_01_rm_011 = 0 + 8 + 3,
    mod_01_rm_100 = 0 + 8 + 4,
    mod_01_rm_101 = 0 + 8 + 5,
    mod_01_rm_110 = 0 + 8 + 6,
    mod_01_rm_111 = 0 + 8 + 7,

    mod_10_rm_000 = 0 + 16 + 0,
    mod_10_rm_001 = 0 + 16 + 1,
    mod_10_rm_010 = 0 + 16 + 2,
    mod_10_rm_011 = 0 + 16 + 3,
    mod_10_rm_100 = 0 + 16 + 4,
    mod_10_rm_101 = 0 + 16 + 5,
    mod_10_rm_110 = 0 + 16 + 6,
    mod_10_rm_111 = 0 + 16 + 7,

    mod_11_rm_000 = 0 + 24 + 0,
    mod_11_rm_001 = 0 + 24 + 1,
    mod_11_rm_010 = 0 + 24 + 2,
    mod_11_rm_011 = 0 + 24 + 3,
    mod_11_rm_100 = 0 + 24 + 4,
    mod_11_rm_101 = 0 + 24 + 5,
    mod_11_rm_110 = 0 + 24 + 6,
    mod_11_rm_111 = 0 + 24 + 7,

    mod_11_rm_000_w = 8 + 24 + 0,
    mod_11_rm_001_w = 8 + 24 + 1,
    mod_11_rm_010_w = 8 + 24 + 2,
    mod_11_rm_011_w = 8 + 24 + 3,
    mod_11_rm_100_w = 8 + 24 + 4,
    mod_11_rm_101_w = 8 + 24 + 5,
    mod_11_rm_110_w = 8 + 24 + 6,
    mod_11_rm_111_w = 8 + 24 + 7,
};


static RegisterMemory calc_rm(int w_bits1, int mod_bits2, int rm_bits3)
{
    auto mod = mod_bits2 * 3;
    auto rm = rm_bits3;

    auto w = (w_bits1 && mod == 24) ? 8 : 0;    

    return (RegisterMemory)(w + mod + rm);
}


static cstr decode_register_memory(RegisterMemory rm)
{
    using RM = RegisterMemory;

    switch (rm)
    {
    case RM::mod_00_rm_000: return "bx + si";
    case RM::mod_00_rm_001: return "bx + di";
    case RM::mod_00_rm_010: return "bp + si";
    case RM::mod_00_rm_011: return "bp + di";
    case RM::mod_00_rm_100: return "si";
    case RM::mod_00_rm_101: return "di";
    case RM::mod_00_rm_110: return "DIRECT ADDRESS";
    case RM::mod_00_rm_111: return "bx";

    case RM::mod_01_rm_000: return "bx + si +";
    case RM::mod_01_rm_001: return "bx + di +";
    case RM::mod_01_rm_010: return "bp + si +";
    case RM::mod_01_rm_011: return "bp + di +";
    case RM::mod_01_rm_100: return "si +";
    case RM::mod_01_rm_101: return "di +";
    case RM::mod_01_rm_110: return "bp +";
    case RM::mod_01_rm_111: return "bx +";

    case RM::mod_10_rm_000: return "bx + si +";
    case RM::mod_10_rm_001: return "bx + di +";
    case RM::mod_10_rm_010: return "bp + si +";
    case RM::mod_10_rm_011: return "bp + di +";
    case RM::mod_10_rm_100: return "si +";
    case RM::mod_10_rm_101: return "di +";
    case RM::mod_10_rm_110: return "bp +";
    case RM::mod_10_rm_111: return "bx +";

    case RM::mod_11_rm_000: return "al";
    case RM::mod_11_rm_001: return "cl";
    case RM::mod_11_rm_010: return "dl";
    case RM::mod_11_rm_011: return "bl";
    case RM::mod_11_rm_100: return "ah";
    case RM::mod_11_rm_101: return "ch";
    case RM::mod_11_rm_110: return "dh";
    case RM::mod_11_rm_111: return "bh";

    case RM::mod_11_rm_000_w: return "ax";
    case RM::mod_11_rm_001_w: return "cx";
    case RM::mod_11_rm_010_w: return "dx";
    case RM::mod_11_rm_011_w: return "bx";
    case RM::mod_11_rm_100_w: return "sp";
    case RM::mod_11_rm_101_w: return "bp";
    case RM::mod_11_rm_110_w: return "si";
    case RM::mod_11_rm_111_w: return "di";
    }

    return "";
}


static void print_register_memory(int w_bits1, int mod_bits2, int rm_bits3, int disp, char* dst)
{
    using RM = RegisterMemory;

    auto rm = calc_rm(w_bits1, mod_bits2, rm_bits3);
    auto rm_int = (int)rm;

    auto rm_str = decode_register_memory(rm);

    // error
    if (!strlen(rm_str))
    {
        snprintf(dst, 4, "rm?");
        return;
    }

    auto len = strlen(rm_str) + 1;

    // register
    if (rm_int >= (int)RM::mod_11_rm_000)
    {
        
        snprintf(dst, len, "%s", rm_str);
        return;
    }

    // direct address
    if (rm_int == (int)RM::mod_00_rm_110)
    {
        
        
        return;
    }

    // Source address calculation no displacement
    if (rm_int <= (int)RM::mod_00_rm_111)
    {        
        snprintf(dst, len + 2, "[%s]", rm_str);
        return;
    }

    // Source address calculation, 8 bit displacement
    /*if (rm_int <= (int)RM::mod_01_rm_111)
    {        
        snprintf(dst, len + 2, "[%s %d]", rm_str, disp);
        return;
    }*/

    // Source address calculation, 8 or 16 bit displacement
    if (rm_int <= (int)RM::mod_10_rm_111)
    {        
        snprintf(dst, len + 2, "[%s %d]", rm_str, disp);
        return;
    }

    // error
    snprintf(dst, 4, "rm?");
}


static OpCode parse_opcode(u8 byte)
{
    using OC = OpCode;

    constexpr int top4 = 0b1111'0000;
    constexpr int top6 = 0b1111'1100;
    constexpr int top7 = 0b1111'1110;

    if (byte & top6 == (int)OC::mov_rm_tf_r)
    {
        return OC::mov_rm_tf_r;
    }
    else if (byte & top7 == (int)OC::mov_i_to_rm)
    {
        return OC::mov_i_to_rm;
    }
    else if (byte & top4 == (int)OC::mov_i_to_r)
    {
        return OC::mov_i_to_r;
    }
    else if (byte & top7 == (int)OC::mov_m_to_a)
    {
        return OC::mov_m_to_a;
    }
    else if (byte & top7 == (int)OC::mov_m_to_a)
    {
        return OC::mov_a_to_m;
    }

    return OC::none;
}

/*
static Dir parse_dir(u8 byte, OpCode opcode)
{
    if (opcode != OpCode::rm_tf_r)
    {
        return Dir::none;
    }

    if (byte & 0b0000'0010)
    {
        return Dir::to_r;
    }

    return Dir::from_r;
}


static Width parse_width(u8 byte, OpCode opcode)
{
    switch (opcode)
    {
    case OpCode::rm_tf_r:
    case OpCode::i_to_rm:
    case OpCode::m_to_a:
    case OpCode::a_to_m:
        return byte & 0b0000'0001 ? Width::word : Width::byte;
    
    case OpCode::i_to_r:
        return byte & 0b0000'1000 ? Width::word : Width::byte;
    }

    return Width::none;
}
*/

static Mode parse_mode(u8 byte2, OpCode opcode)
{
    using OC = OpCode;

    switch (opcode)
    {
    case OC::mov_rm_tf_r:
    case OC::mov_i_to_rm:
        return (Mode)((byte2 & 0b1100'0000) >> 6);
    }

    return Mode::none;
}


static cstr decode_register(int bits3, int w)
{
    using R = Register;

    assert (bits3 >= 0);
    assert(bits3 < 8);

    auto reg = (R)(w ? bits3 + 8 : bits3);

    switch (reg)
    {
    case R::AL: return "al";
    case R::CL: return "cl";
    case R::DL: return "dl";
    case R::BL: return "bl";
    case R::AH: return "ah";
    case R::CH: return "ch";
    case R::DH: return "dh";
    case R::BH: return "bh";
    case R::AX: return "ax";
    case R::CX: return "cx";
    case R::DX: return "dx";
    case R::BX: return "bx";
    case R::SP: return "sp";
    case R::BP: return "bp";
    case R::SI: return "si";
    case R::DI: return "di";
    }

    return "reg?";
}


static cstr decode_rm(Mode mod, int bits3, int w)
{
    switch (mod)
    {
    case Mode::mem:
        break;
    case Mode::mem_8:
        break;
    case Mode::mem_16:
        break;
    case Mode::reg:
        return decode_register(bits3, w);
    default:
        break;
    }
}


static void decode_mov_m8_r(u8 byte1, u8 byte2)
{
    auto d = byte1 & 0b0000'0010;
    auto w = byte1 & 0b0000'0001;

    int src_bits3 = 0;
    int dst_bits3 = 0;

    auto src = "src?";
    auto dst = "dst?";

    if (d)
    {
        src_bits3 = byte2 & 0b00'000'111;
        dst_bits3 = (byte2 & 0b00'111'000) >> 3;

        dst = decode_register(dst_bits3, w);
    }
    else
    {
        src_bits3 = (byte2 & 0b00'111'000) >> 3;
        dst_bits3 = byte2 & 0b00'000'111;

        src = decode_register(src_bits3, w);
    }

    printf("mov %s, %s\n", dst, src);
}


static void decode_mov_r_r(u8 byte1, u8 byte2)
{
    auto d = byte1 & 0b0000'0010;
    auto w = byte1 & 0b0000'0001;

    int src_bits3 = 0;
    int dst_bits3 = 0;

    if (d)
    {
        src_bits3 = byte2 & 0b00'000'111;
        dst_bits3 = (byte2 & 0b00'111'000) >> 3;
    }
    else
    {
        src_bits3 = (byte2 & 0b00'111'000) >> 3;
        dst_bits3 = byte2 & 0b00'000'111;
    }

    auto src = decode_register(src_bits3, w);
    auto dst = decode_register(dst_bits3, w);

    printf("mov %s, %s\n", dst, src);
}


static int decode_mov_rm_tf_r(u8* data, OpCode opcode, int offset)
{
    auto byte1 = data[offset];
    auto byte2 = data[offset + 1];

    auto mode = parse_mode(byte1, opcode);

    switch (mode)
    {
    case Mode::mem:
        break;
    case Mode::mem_8:
        break;
    case Mode::mem_16:
        break;
    case Mode::reg:
        decode_mov_r_r(byte1, byte2);
        offset += 2;
        break;    
    default:
        break;
    }

    return offset;
}


static int decode_i_to_rm(u8* data, OpCode opcode, Width width, int offset)
{
    auto byte1 = data[offset];
    auto byte2 = data[offset + 1];

    auto mode = parse_mode(byte1, opcode);

    switch (mode)
    {
    case Mode::mem:
        break;
    case Mode::mem_8:
        break;
    case Mode::mem_16:
        break;
    case Mode::reg:
        break;    
    default:
        break;
    }

    return offset;
}


static int decode_i_to_r()
{

}


static int decode_m_to_a()
{

}


static int decode_a_to_m()
{

}


static int decode_next(u8* data, int offset)
{
    using OC = OpCode;

    auto byte = data[offset];

    auto opcode = parse_opcode(byte);

    switch (opcode)
    {
    case OC::mov_rm_tf_r:
        offset = decode_mov_rm_tf_r(data, opcode, offset);
        break;
    case OC::mov_i_to_rm:

        break;
    case OC::mov_i_to_r:

        break;
    case OC::mov_m_to_a:

        break;
    case OC::mov_a_to_m:

        break;
    default:
        printf("opcode: %d\n", (int)opcode);
        return offset;
    }

    


    return offset;
}



constexpr auto BIN_FILE = "listing_0039_more_movs";
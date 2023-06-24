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


#define PRINTDBG


static void printdbg(cstr msg)
{
#ifdef PRINTDBG

printf("%s", msg);

#endif
}


namespace Bytes
{
    class Buffer
    {
    public:
        u32 size = 0;
        u8* data = nullptr;
    };


    static void destroy(Buffer& buffer)
    {
        if (buffer.data)
        {
            std::free(buffer.data);
        }
    }


    static bool create(Buffer& buffer, u32 size)
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


    static Buffer read(fs::path const& path)
    {
        Buffer buffer;

        auto size = fs::file_size(path);
        if (size == 0 || !create(buffer, size))
        {
            assert(false);
            return buffer;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            assert(false);
            destroy(buffer);
            return buffer;
        }

        file.read((char*)buffer.data, size);
        buffer.size = size;

        file.close();
        return buffer;
    }

}


namespace Reg
{
    enum class Name : int
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


    static cstr decode(int reg_bits3, int w)
    {
        using R = Reg::Name;

        assert(reg_bits3 >= 0);
        assert(reg_bits3 < 8);

        auto reg = (R)(w ? reg_bits3 + 8 : reg_bits3);

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
}


namespace RegMem
{
    enum class Name : int
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


    static RegMem::Name calc_rm(int w_bits1, int mod_bits2, int rm_bits3)
    {
        auto mod = mod_bits2 * 8;
        auto rm = rm_bits3;

        auto w = (w_bits1 && mod == 24) ? 8 : 0;    

        return (RegMem::Name)(w + mod + rm);
    }


    static cstr decode(RegMem::Name rm)
    {
        using RM = RegMem::Name;

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

        // 8 bit displacement
        case RM::mod_01_rm_000: return "bx + si";
        case RM::mod_01_rm_001: return "bx + di";
        case RM::mod_01_rm_010: return "bp + si";
        case RM::mod_01_rm_011: return "bp + di";
        case RM::mod_01_rm_100: return "si";
        case RM::mod_01_rm_101: return "di";
        case RM::mod_01_rm_110: return "bp";
        case RM::mod_01_rm_111: return "bx";

        // 16 bit displacement
        case RM::mod_10_rm_000: return "bx + si";
        case RM::mod_10_rm_001: return "bx + di";
        case RM::mod_10_rm_010: return "bp + si";
        case RM::mod_10_rm_011: return "bp + di";
        case RM::mod_10_rm_100: return "si";
        case RM::mod_10_rm_101: return "di";
        case RM::mod_10_rm_110: return "bp";
        case RM::mod_10_rm_111: return "bx";

        // register, w = 0
        case RM::mod_11_rm_000: return "al";
        case RM::mod_11_rm_001: return "cl";
        case RM::mod_11_rm_010: return "dl";
        case RM::mod_11_rm_011: return "bl";
        case RM::mod_11_rm_100: return "ah";
        case RM::mod_11_rm_101: return "ch";
        case RM::mod_11_rm_110: return "dh";
        case RM::mod_11_rm_111: return "bh";

        // register, w = 1
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


    static int decode_disp(RegMem::Name rm)
    {
        using RM = RegMem::Name;

        auto rm_int = (int)rm;

        if (rm_int == (int)RM::mod_00_rm_110)
        {
            return 2;
        }

        if (rm_int <= (int)RM::mod_00_rm_111)
        {
            return 0;
        }

        if (rm_int <= (int)RM::mod_01_rm_111)
        {
            return 1;
        }

        if (rm_int <= (int)RM::mod_10_rm_111)
        {
            return 2;
        }

        return 0;
    }


    static void print(RegMem::Name rm, int disp, char* dst)
    {
        using RM = RegMem::Name;
        
        auto rm_int = (int)rm;

        auto rm_str = decode(rm);

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
        if (rm == RM::mod_00_rm_110)
        {
            char buffer[7] = { 0 };
            snprintf(buffer, 7, "%d", disp);
            snprintf(dst, 9, "[%s]", buffer);
            return;
        }

        // Source address calculation no displacement
        if (rm_int <= (int)RM::mod_00_rm_111)
        {        
            snprintf(dst, len + 2, "[%s]", rm_str);
            return;
        }

        // Source address calculation, 8 or 16 bit displacement
        if (rm_int <= (int)RM::mod_10_rm_111)
        {        
            char buffer[7] = { 0 };

            if (disp < 0)
            {
                snprintf(buffer, 7, "%d", -1 * disp);
                snprintf(dst, len + 8, "[%s + %s]", rm_str, buffer);
            }
            else if (disp > 0)
            {
                snprintf(buffer, 7, "%d", disp);
                snprintf(dst, len + 11, "[%s + %s]", rm_str, buffer);
            }
            else
            {
                snprintf(dst, len + 11, "[%s]", rm_str);
            }
            
            return;
        }

        // error
        snprintf(dst, 4, "rm?");
    }

}


namespace OpCode
{
    enum class Name : int
    {
        mov_rm_tf_r,
        mov_i_to_rm,
        mov_i_to_r,
        mov_m_to_ac,
        mov_ac_to_m,
        none = -1
    };


    static OpCode::Name parse(u8 byte)
    {
        using OC = OpCode::Name;

        auto top4 = byte >> 4;
        auto top6 = byte >> 2;
        auto top7 = byte >> 1;

        if (top6 == 0b0010'0010)
        {
            return OC::mov_rm_tf_r;
        }
        else if (top7 == 0b0110'0011)
        {
            return OC::mov_i_to_rm;
        }
        else if (top4 == 0b0000'1011)
        {
            return OC::mov_i_to_r;
        }
        else if (top7 == 0b0101'0000)
        {
            return OC::mov_m_to_ac;
        }
        else if (top7 == 0b01010001)
        {
            return OC::mov_ac_to_m;
        }

        return OC::none;
    }
}


namespace MOV
{
    namespace R = Reg;
    namespace RM = RegMem;

    static int rm_tf_r(u8* data, int offset)
    {
        printdbg("mov rm_tf_r: ");

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto d_bits1 = (byte1 & 0b0000'0010) >> 1;
        auto w_bits1 = byte1 & 0b0000'0001;

        auto mod_bits2 = (byte2 & 0b1100'0000) >> 6;
        auto reg_bits3 = (byte2 & 0b0011'1000) >> 3;
        auto rm_bits3 = byte2 & 0b0000'0111;

        auto reg_str = R::decode(reg_bits3, w_bits1);

        auto rm = RM::calc_rm(w_bits1, mod_bits2, rm_bits3);
        auto rm_disp = RM::decode_disp(rm);

        int disp = 0;
        offset += 2;

        auto byte3 = data[offset];

        switch (rm_disp)
        {
        case 1:
            disp = (int)byte3;
            offset += 1;
            break;
        case 2:
            auto byte4 = data[offset + 1];
            disp = (byte4 << 8) + byte3;
            offset += 2;
            break;
        }

        char rm_str[20] = { 0 };
        RM::print(rm, disp, rm_str);

        auto src = "src?";
        auto dst = "dst?";

        if (d_bits1)
        {
            src = rm_str;
            dst = reg_str;
        }
        else
        {
            src = reg_str;
            dst = rm_str;
        }

        printf("mov %s, %s\n", dst, src);

        return offset;
    }


    static int i_to_rm(u8* data, int offset)
    {
        printdbg("mov i_to_rm: ");

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto w_bits1 = byte1 & 0b0000'0001;
        auto mod_bits2 = (byte2 & 0b1100'0000) >> 6;
        auto rm_bits3 = byte2 & 0b0000'0111;

        auto rm = RM::calc_rm(w_bits1, mod_bits2, rm_bits3);
        auto rm_disp = RM::decode_disp(rm);

        int disp = 0;
        auto disp_str = "byte";
        offset += 2;

        auto byte3 = data[offset];

        switch (rm_disp)
        {
        case 1:
            disp = (int)byte3;
            offset += 1;
            break;
        case 2:
            auto byte4 = data[offset + 1];
            disp = (byte4 << 8) + byte3;
            disp_str = "word";
            offset += 2;
            break;
        }

        char rm_str[20] = { 0 };
        RM::print(rm, disp, rm_str);

        int im_data = (int)(data[offset]);
        offset += 1;

        if (w_bits1)
        {
            im_data += (data[offset] << 8);
            offset += 1;
        }

        char src[6] = { 0 };
        snprintf(src, 6, "%d", im_data);

        auto dst = rm_str;

        printf("mov %s, %s %s\n", dst, disp_str, src);

        return offset;
    }


    static int i_to_r(u8* data, int offset)
    {
        printdbg("mov i_to_r:  ");

        auto byte1 = data[offset];

        auto w_bits1 = (byte1 & 0b0000'1000) >> 3;
        auto reg_bits3 = byte1 & 0b0000'0111;
        
        int im_data = (int)(data[offset + 1]);
        
        if (w_bits1)
        {
            im_data += (data[offset + 2] << 8);
            offset += 1;
        }

        offset += 2;

        char src[6] = { 0 };
        snprintf(src, 6, "%d", im_data);

        auto dst = R::decode(reg_bits3, w_bits1);

        printf("mov %s, %s\n", dst, src);

        return offset;
    }


    static int m_to_ac(u8* data, int offset)
    {
        printdbg("mov m_to_ac: ");

        auto byte1 = data[offset];

        auto w_bits1 = byte1 & 0b0000'0001;

        int addr = (int)(data[offset + 1]);

        if (w_bits1)
        {
            addr += (data[offset + 2] << 8);
            offset += 1;
        }

        offset += 2;

        char src[8] = { 0 };
        snprintf(src, 8, "[%d]", addr);

        auto dst = "ax";

        printf("mov %s, %s\n", dst, src);

        return offset;
    }


    static int ac_to_m(u8* data, int offset)
    {
        printdbg("mov ac_to_m: ");

        auto byte1 = data[offset];

        auto w_bits1 = byte1 & 0b0000'0001;

        int addr = (int)(data[offset + 1]);

        if (w_bits1)
        {
            addr += (data[offset + 2] << 8);
            offset += 1;
        }

        offset += 2;

        auto src = "ax";

        char dst[8] = { 0 };
        snprintf(dst, 8, "[%d]", addr);

        printf("mov %s, %s\n", dst, src);

        return offset;
    }

}


static int decode_next(u8* data, int offset)
{
    using OC = OpCode::Name;

    auto byte = data[offset];

    auto opcode =  OpCode::parse(byte);

    switch (opcode)
    {
    case OC::mov_rm_tf_r:        
        offset = MOV::rm_tf_r(data, offset);
        break;
    case OC::mov_i_to_rm:
        offset = MOV::i_to_rm(data, offset);
        break;
    case OC::mov_i_to_r:
        offset = MOV::i_to_r(data, offset);
        break;
    case OC::mov_m_to_ac:
        offset = MOV::m_to_ac(data, offset);
        break;
    case OC::mov_ac_to_m:
        offset = MOV::ac_to_m(data, offset);
        break;
    default:
        printf("opcode (byte1): %d\n", (int)data[offset]);
        return -1;
    }

    return offset;
}


static void decode(cstr bin_file)
{
    printf("\n==================\n\n");

    auto buffer = Bytes::read(bin_file);

    assert(buffer.data);
    assert(buffer.size);

    printf("bits 16\n\n");

    int offset = 0;
    while (offset >= 0 && offset < buffer.size)
    {
        offset = decode_next(buffer.data, offset);
    }

    destroy(buffer);

    printf("\n\n------------------\n");
}


int main()
{
    decode("../02/listing_0039_more_movs");

    decode("../02/listing_0040_challenge_movs");
}
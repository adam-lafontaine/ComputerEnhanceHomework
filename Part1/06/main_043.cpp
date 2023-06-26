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


class Cstr
{
public:
    char str[10] = { 0 };
    constexpr static u32 length = 10;
};


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


namespace ASM
{
    class InstStr
    {
    public:
        Cstr op;
        Cstr src;
        Cstr dst;

        bool is_valid;
    };


    static void print(InstStr const& inst)
    {
        printf("%s %s %s", inst.op.str, inst.dst.str, inst.src.str);
    }


    static int next_line(u8* data, int offset)
    {
        while (data[offset++] != '\n'){}

        return offset;
    }


    static int read_line(char* line, u8* data, int offset)
    {
        u32 i = 0;
        while (data[offset] != '\n') 
        {
            line[i++] = (char)data[offset++];
        }

        return offset;
    }


    static int read_next(InstStr& inst, u8* data, int offset)
    {
        inst.is_valid = false;

        switch((char)data[offset])
        {
        case ';':  return next_line(data, offset);
        case '\n': return offset + 1;
        }
        
        char line[20] = { 0 };
        offset = read_line(line, data, offset);

        if (strcmp(line, "bits 16") == 0)
        {
            return offset;
        }

        u32 i = 0;
        u32 s = 0;
        while (line[i] != ' ')
        {
            inst.op.str[s++] = line[i++];
        }

        i++;
        s = 0;
        while (line[i] != ' ')
        {
            inst.dst.str[s++] = line[i++];
        }

        i++;
        s = 0;
        while (line[i])
        {
            inst.src.str[s++] = line[i++];
        }

        inst.is_valid = true;

        return offset;
    }


    static void print_file(cstr asm_file)
    {
        auto buffer = Bytes::read(asm_file);
        if (!buffer.data)
        {
            printf("error %s", asm_file);
        }

        int offset = 0;
        while (offset < buffer.size)
        {
            InstStr inst{};
            offset = read_next(inst, buffer.data, offset);
            if (inst.is_valid)
            {
                print(inst);
                printf("\n");
            }
        }

        Bytes::destroy(buffer);
    }
}


namespace REG
{
    constexpr int HI_8 = 0b1111'1111'0000'0000;
    constexpr int LOW_8 = 0b0000'0000'1111'1111;

    static u16 AX = 0;
    static u16 BX = 0;
    static u16 CX = 0;
    static u16 DX = 0;
    static u16 SP = 0;
    static u16 BP = 0;
    static u16 SI = 0;
    static u16 DI = 0;


    static int ax() { return (int)AX; }
    static int bx() { return (int)BX; }
    static int cx() { return (int)CX; }
    static int dx() { return (int)DX; }    

    static int ah() { return AX >> 8; }
    static int bh() { return BX >> 8; }
    static int ch() { return CX >> 8; }
    static int dh() { return DX >> 8; }

    static int al() { return AX & LOW_8; }
    static int bl() { return BX & LOW_8; }
    static int cl() { return CX & LOW_8; }
    static int dl() { return DX & LOW_8; }

    static int sp() { return (int)SP; }
    static int bp() { return (int)BP; }
    static int si() { return (int)SI; }
    static int di() { return (int)DI; }


    enum class Name : int
    {
        ax,
        bx,
        cx,
        dx,

        ah,
        bh,
        ch,
        dh,

        al,
        bl,
        cl,
        dl,
        
        sp,
        bp,
        si,
        di,

        none = -1
    };


    static int get(Name r)
    {
        using R = REG::Name;

        switch (r)
        {
        case R::ax: return ax();
        case R::bx: return bx();
        case R::cx: return cx();
        case R::dx: return dx();
        
        case R::ah: return ah();
        case R::bh: return bh();
        case R::ch: return ch();
        case R::dh: return dh();

        case R::al: return al();
        case R::bl: return bl();
        case R::cl: return cl();
        case R::dl: return dl();

        case R::sp: return ax();
        case R::bp: return ax();
        case R::si: return ax();
        case R::di: return ax();
        }

        return -1;
    }


    static void print_all()
    {
        auto const print = [](cstr str, int val){ printf("%s: 0x%04x (%d)\n", str, val, val); };

        print("ax", ax());
        print("bx", cx());
        print("cx", cx());
        print("dx", dx());
        print("sp", sp());
        print("bp", bp());
        print("si", si());
        print("di", di());
    }       
}


namespace MOV
{
    static void ax(int v) { REG::AX = (u16)v; }
    static void bx(int v) { REG::BX = (u16)v; }
    static void cx(int v) { REG::CX = (u16)v; }
    static void dx(int v) { REG::DX = (u16)v; }

    static void ah(int v) { REG::AX = (u16)(((v & REG::LOW_8) << 8) + (REG::AX & REG::LOW_8)); }
    static void bh(int v) { REG::AX = (u16)(((v & REG::LOW_8) << 8) + (REG::BX & REG::LOW_8)); }
    static void ch(int v) { REG::AX = (u16)(((v & REG::LOW_8) << 8) + (REG::CX & REG::LOW_8)); }
    static void dh(int v) { REG::AX = (u16)(((v & REG::LOW_8) << 8) + (REG::DX & REG::LOW_8)); }

    static void al(int v) { REG::AX = (u16)((REG::AX & REG::HI_8) + (v & REG::LOW_8)); }
    static void bl(int v) { REG::BX = (u16)((REG::BX & REG::HI_8) + (v & REG::LOW_8)); }
    static void cl(int v) { REG::CX = (u16)((REG::CX & REG::HI_8) + (v & REG::LOW_8)); }
    static void dl(int v) { REG::DX = (u16)((REG::DX & REG::HI_8) + (v & REG::LOW_8)); }

    static void sp(int v) { REG::SP = (u16)v; }
    static void bp(int v) { REG::BP = (u16)v; }
    static void si(int v) { REG::SI = (u16)v; }
    static void di(int v) { REG::DI = (u16)v; }
}


namespace OP
{
    enum class Name : int
    {
        mov,
        add,
        sub,
        cmp,

        none = -1
    };


    Name get_op(ASM::InstStr const& inst)
    {
        using Op = OP::Name;

        if (!inst.is_valid)
        {
            return Op::none;
        }

        auto str = inst.op.str;

        if (strcmp(str, "mov"))
        {
            return Op::mov;
        }

        return Op::none;
    }

}


int main()
{
    ASM::print_file("listing_0043_immediate_movs.asm");

    printf("Final registers:\n");
    REG::print_all();
}
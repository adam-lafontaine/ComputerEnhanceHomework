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


    static int get_value(Name r)
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

        case R::sp: return sp();
        case R::bp: return bp();
        case R::si: return si();
        case R::di: return di();
        }

        return -1;
    }


    static Name get_reg(cstr str)
    {
        using R = REG::Name;        

        if (strncmp(str, "ax", 2) == 0)
        {
            return R::ax;
        }
        else if (strncmp(str, "bx", 2) == 0)
        {
            return R::bx;
        }
        else if (strncmp(str, "cx", 2) == 0)
        {
            return R::cx;
        }
        else if (strncmp(str, "dx", 2) == 0)
        {
            return R::dx;
        }
        else if (strncmp(str, "ah", 2) == 0)
        {
            return R::ah;
        }
        else if (strncmp(str, "bh", 2) == 0)
        {
            return R::bh;
        }
        else if (strncmp(str, "ch", 2) == 0)
        {
            return R::ch;
        }
        else if (strncmp(str, "dh", 2) == 0)
        {
            return R::dh;
        }
        else if (strncmp(str, "al", 2) == 0)
        {
            return R::al;
        }
        else if (strncmp(str, "bl", 2) == 0)
        {
            return R::bl;
        }
        else if (strncmp(str, "cl", 2) == 0)
        {
            return R::cl;
        }
        else if (strncmp(str, "dl", 2) == 0)
        {
            return R::dl;
        }
        else if (strncmp(str, "sp", 2) == 0)
        {
            return R::sp;
        }
        else if (strncmp(str, "bp", 2) == 0)
        {
            return R::bp;
        }
        else if (strncmp(str, "si", 2) == 0)
        {
            return R::si;
        }
        else if (strncmp(str, "di", 2) == 0)
        {
            return R::di;
        }

        return R::none;
    }


    static Name get_reg_dst(ASM::InstStr const& inst)
    {
        using R = REG::Name;

        if (!inst.is_valid)
        {
            return R::none;
        }

        return get_reg(inst.dst.str);
    }


    static Name get_reg_src(ASM::InstStr const& inst)
    {
        using R = REG::Name;

        if (!inst.is_valid)
        {
            return R::none;
        }

        return get_reg(inst.src.str);
    }


    static void print_all()
    {
        auto const print = [](cstr str, int val){ printf("%s: 0x%04x (%d)\n", str, val, val); };

        print("ax", ax());
        print("bx", bx());
        print("cx", cx());
        print("dx", dx());
        print("sp", sp());
        print("bp", bp());
        print("si", si());
        print("di", di());
    }


    static void reset()
    {
        AX = 0;
        BX = 0;
        CX = 0;
        DX = 0;
        SP = 0;
        BP = 0;
        SI = 0;
        DI = 0;
    }
}


namespace MOV
{
    typedef void (*func_t)(int);

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

    static void no_op(int) {}


    static func_t get_mov_f(REG::Name reg)
    {
        using R = REG::Name;

        switch (reg)
        {
        case R::ax: return ax;
        case R::bx: return bx;
        case R::cx: return cx;
        case R::dx: return dx;
        
        case R::ah: return ah;
        case R::bh: return bh;
        case R::ch: return ch;
        case R::dh: return dh;

        case R::al: return al;
        case R::bl: return bl;
        case R::cl: return cl;
        case R::dl: return dl;

        case R::sp: return sp;
        case R::bp: return bp;
        case R::si: return si;
        case R::di: return di;
        }

        return no_op;
    }


    static void do_mov(ASM::InstStr const& inst)
    {
        auto reg_dst = REG::get_reg_dst(inst);
        auto v1 = REG::get_value(reg_dst);
        auto reg_src = REG::get_reg_src(inst);
        auto val = reg_src == REG::Name::none ? atoi(inst.src.str) : REG::get_value(reg_src);
        auto mov = MOV::get_mov_f(reg_dst);
        mov(val);
        auto v2 = REG::get_value(reg_dst);

        printf("%s:0x%x->0x%x", inst.dst.str, v1, v2);
    }
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

        if (strcmp(str, "mov") == 0)
        {
            return Op::mov;
        }

        return Op::none;
    }

}


static void eval(ASM::InstStr const& inst)
{
    using Op = OP::Name;

    ASM::print(inst);
    printf(" ; ");

    auto op = OP::get_op(inst);

    switch (op)
    {
    case Op::mov:
        MOV::do_mov(inst);
        break;
    default:
        printf("error");
    }
    

    printf("\n");
}


static void eval_file(cstr asm_file)
{
    auto buffer = Bytes::read(asm_file);
    if (!buffer.data)
    {
        printf("error %s", asm_file);
    }

    int offset = 0;
    while (offset < buffer.size)
    {
        ASM::InstStr inst{};
        offset = ASM::read_next(inst, buffer.data, offset);
        if (!inst.is_valid)
        {
            continue;
        }

        eval(inst);
    }

    Bytes::destroy(buffer);
}


int main()
{
    constexpr auto file_043 = "listing_0043_immediate_movs.asm";
    constexpr auto file_044 = "listing_0044_register_movs.asm";

    eval_file(file_043);

    printf("\nFinal registers:\n");
    REG::print_all();
    printf("\n\n");

    REG::reset();

    eval_file(file_044);;

    printf("\nFinal registers:\n");
    REG::print_all();
}
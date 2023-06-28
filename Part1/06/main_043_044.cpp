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


namespace OP
{
    enum class Name : int
    {
        mov,
        add,
        sub,
        cmp,

        je,
        jl,
        jle,
        jb,
        jbe,
        jp,
        jo,
        js,
        jnz,
        jnl,
        jg,
        jnb,
        ja,
        jnp,
        jno,
        jns,
        loop,
        loopz,
        loopnz,
        jcxz,

        none = -1
    };


    static cstr decode(OP::Name op)
    {
        using Op = OP::Name;

        switch (op)
        {
        case Op::mov: return "mov";
        case Op::add: return "add";
        case Op::sub: return "sub";
        case Op::cmp: return "cmp";

        case Op::je:  return "je";
        case Op::jl:  return "jl";
        case Op::jle: return "jle";
        case Op::jb:  return "jb";
        case Op::jbe: return "jbe";
        case Op::jp:  return "jp";
        case Op::jo:  return "jo";
        case Op::js:  return "js";
        case Op::jnz: return "jnz";
        case Op::jnl: return "jnl";
        case Op::jnb: return "jnb";
        case Op::ja:  return "ja";
        case Op::jnp: return "jnp";
        case Op::jno: return "jno";
        case Op::jns: return "jns";

        case Op::loop:   return "loop";
        case Op::loopz:  return "loopz";
        case Op::loopnz: return "loopnz";
        case Op::jcxz:   return "jcxz";
        }

        return "err";
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


    static cstr decode(Name r)
    {
        using R = REG::Name;

        switch (r)
        {
        case R::ax: return "ax";
        case R::bx: return "bx";
        case R::cx: return "cx";
        case R::dx: return "dx";
        
        case R::ah: return "ah";
        case R::bh: return "bh";
        case R::ch: return "ch";
        case R::dh: return "dh";

        case R::al: return "al";
        case R::bl: return "bl";
        case R::cl: return "cl";
        case R::dl: return "dl";

        case R::sp: return "sp";
        case R::bp: return "bp";
        case R::si: return "si";
        case R::di: return "di";
        }

        return "err";
    }


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


    Name get_reg(int reg_b3, int w)
    {
        using R = REG::Name;

        if (w)
        {
            switch (reg_b3)
            {
            case 0: return R::ax;
            case 1: return R::cx;
            case 2: return R::dx;
            case 3: return R::bx;
            case 4: return R::sp;
            case 5: return R::bp;
            case 6: return R::si;
            case 7: return R::di;
            }
        }
        else
        {
            switch (reg_b3)
            {
            case 0: return R::al;
            case 1: return R::cl;
            case 2: return R::dl;
            case 3: return R::bl;
            case 4: return R::ah;
            case 5: return R::ch;
            case 6: return R::dh;
            case 7: return R::bh;
            }
        }

        return R::none;
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


namespace ASM
{
    using Op = OP::Name;
    using Reg = REG::Name;


    class RegMem
    {
    public:
        Reg reg = Reg::none;
        int mem_offset;
        // effective address
    };


    class RegMemData
    {
    public:
        Reg reg = Reg::none;
        int mem_offset;
        // effective address

        int immediate = 0;
    };


    static void print(RegMem const& rm)
    {
        if (rm.reg == Reg::none)
        {
            printf("[%d]", rm.mem_offset);
        }
        else
        {
            printf("%s", REG::decode(rm.reg));
        }
    }


    static void print(RegMemData const& rmd)
    {

    }
    

    class Instr
    {
    public:
        Op op = Op::none;
        
        RegMem dst;

        RegMemData src;
        
        int offset_begin;
        int offset_end;
    };


    static void print(Instr const& inst)
    {
        if (inst.src.reg != Reg::none)
        {
            printf("%s %s [%d]", decode(inst.op), REG::decode(inst.dst.reg), inst.src.mem_offset);
        }
        else
        {
            printf("%s %s %s", decode(inst.op), REG::decode(inst.dst), REG::decode(inst.src));
        }
    }


    


    static void set_rm_r(Instr& inst, u8* data)
    {
        auto offset = inst.offset_begin;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];
        
        auto w_b1 = byte1 & 0b0000'0001;
        auto mod_b2 = byte2 >> 6;        
        auto rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(mod_b2, rm_b3);
        auto disp = get_disp_val(data + 2, disp_sz);

        inst.offset_end = inst.offset_begin + 2 + disp_sz;

        auto rm = REG::get_reg(rm_b3, w_b1);
        assert(mod_b2 == 0b11);

        auto d_b1 = (byte1 & 0b0000'0010) >> 1;
        auto reg_b3 = (byte2 & 0b00'111'000) >> 3;

        auto reg = REG::get_reg(reg_b3, w_b1);

        if (d_b1)
        {
            inst.dst = reg;
            inst.src = rm;
        }
        else
        {
            inst.dst = rm;
            inst.src = reg;
        }
    }


    static void set_i_rm(Instr& inst, u8* data)
    {
        auto offset = inst.offset_begin;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto w_b1 = byte1 & 0b0000'0001;
        auto mod_b2 = byte2 >> 6;
        auto rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(mod_b2, rm_b3);
        auto disp = get_disp_val(data + 2, disp_sz);

        inst.offset_end = inst.offset_begin + 3 + disp_sz;

        auto rm = REG::get_reg(rm_b3, w_b1);
        assert(mod_b2 == 0b11);

        int im_data = data[offset + 2];

        if (w_b1)
        {
            im_data += (data[offset + 3] << 8);
            inst.offset_end++;
        }

        inst.dst = rm;
        inst.src_val = im_data;
    }


    static void set_i_ac(Instr& inst, u8* data)
    {
        auto offset = inst.offset_begin;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto w = byte1 & 0b0000'0001;
    }


    static void set_mov_rm_r(Instr& inst, u8* data)
    {
        inst.op = Op::mov;
        set_rm_r(inst, data);
    }


    static void set_mov_i_rm(Instr& inst, u8* data)
    {
        inst.op = Op::mov;
        set_i_rm(inst, data);
    }


    static void set_mov_i_r(Instr& inst, u8* data)
    {
        inst.op = Op::mov;

        auto offset = inst.offset_begin;

        auto byte1 = data[offset];

        auto w_b1 = byte1 & 0b0000'1000;
        auto reg_b3 = byte1 & 0b00'000'111;

        auto reg = REG::get_reg(reg_b3, w_b1);

        int im_data = data[offset + 1];
        inst.offset_end = inst.offset_begin + 2;

        if (w_b1)
        {
            im_data += (data[offset + 2] << 8);
            inst.offset_end++;
        }

        inst.dst = reg;
        inst.src_val = im_data;        
    }


    static void set_mov_m_ac(Instr& inst, u8* data)
    {
        inst.op = Op::mov;

        auto offset = inst.offset_begin;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto w_b1 = byte1 & 0b0000'0001;

        int addr = (int)(data[offset + 1]);

        inst.dst = Reg::ax;
        inst.offset_end = inst.offset_begin + 2 + w_b1;        
    }


    static void set_mov_ac_m(Instr& inst, u8* data)
    {
        inst.op = Op::mov;

        auto offset = inst.offset_begin;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        auto w_b1 = byte1 & 0b0000'0001;

        inst.src = Reg::ax;
        inst.offset_end = inst.offset_begin + 2 + w_b1;
    }


    static Instr decode_next(u8* data, int offset)
    {
        constexpr int add_b3 = 0b000;
        constexpr int sub_b3 = 0b101;
        constexpr int cmp_b3 = 0b111;

        Instr inst{};
        inst.offset_begin = offset;

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];        

        auto byte1_top4 = byte1 >> 4;
        auto byte1_top6 = byte1 >> 2;
        auto byte1_top7 = byte1 >> 1;

        auto byte2_345 = (byte2 & 0b00'111'000) >> 3;

        auto is_mov = 
            byte1_top4 == 0b0000'1011 || 
            byte1_top6 == 0b0010'0010 || 
            byte1_top7 == 0b0110'0011 ||
            byte1_top7 == 0b0101'0000 ||
            byte1_top7 == 0b0101'0001;

        auto is_add = 
            byte1_top6 == 0b0000'0000 ||
            byte1_top7 == 0b0000'0010 ||
            (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0000);
        
        auto is_sub = 
            byte1_top6 == 0b0000'1010 ||
            byte1_top7 == 0b0001'0110 ||
            (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0101);
        
        auto is_cmp = 
            byte1_top6 == 0b0000'1110 ||
            byte1_top7 == 0b0001'1110 ||
            (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0111);
        
        auto const set_jump = [&](Op op)
        {
            inst.op = op;
            inst.src_val = (int)byte2;
            inst.offset_end = offset + 2;
        };

        if (is_mov)
        {
            inst.op = Op::mov;
        }
        else if (is_add)
        {
            inst.op = Op::add;
        }
        else if (is_sub)
        {
            inst.op = Op::sub;
        }
        else if (is_cmp)
        {
            inst.op = Op::cmp;
        }
        else if (byte1 == 0b0111'0100)
        {
            set_jump(Op::je);
        }
        else if (byte1 == 0b0111'1100)
        {
            set_jump(Op::jl);
        }
        else if (byte1 == 0b0111'1110)
        {
            set_jump(Op::jle);
        }
        else if (byte1 == 0b0111'0010)
        {
            set_jump(Op::jb);
        }
        else if (byte1 == 0b0111'0110)
        {
            set_jump(Op::jbe);
        }
        else if (byte1 == 0b0111'1010)
        {
            set_jump(Op::jp);
        }
        else if (byte1 == 0b0111'0000)
        {
            set_jump(Op::jo);
        }
        else if (byte1 == 0b0111'1000)
        {
            set_jump(Op::js);
        }
        else if (byte1 == 0b0111'0101)
        {
            set_jump(Op::jnz);
        }
        else if (byte1 == 0b0111'1101)
        {
            set_jump(Op::jnl);
        }
        else if (byte1 == 0b0111'1111)
        {
            set_jump(Op::jg);
        }
        else if (byte1 == 0b0111'0011)
        {
            set_jump(Op::jnb);
        }
        else if (byte1 == 0b0111'0111)
        {
            set_jump(Op::ja);
        }
        else if (byte1 == 0b0111'1011)
        {
            set_jump(Op::jnp);
        }
        else if (byte1 == 0b0111'0001)
        {
            set_jump(Op::jno);
        }
        else if (byte1 == 0b0111'1001)
        {
            set_jump(Op::jns);
        }
        else if (byte1 == 0b1110'0010)
        {
            set_jump(Op::loop);
        }
        else if (byte1 == 0b1110'0001)
        {
            set_jump(Op::loopz);
        }
        else if (byte1 == 0b1110'0000)
        {
            set_jump(Op::loopnz);
        }
        else if (byte1 == 0b1110'0011)
        {
            set_jump(Op::jcxz);
        }
        else
        {
            inst.op = Op::none;
        }





        return inst;
    }
}


namespace CMD
{
    class Instr
    {
    public:
        int src_reg_b3 = -1;
        int src_mem_b3 = -1;        

        int src_imlo_b8 = -1;
        int src_imhi_b8 = -1;

        int dst_reg_b3 = -1;
        int dst_mem_b3 = -1;

        int displo_b8 = -1;
        int disphi_b8 = -1;

        int offset_begin = 0;
        int offset_end = 0;
    };


    class InstrData
    {
    public:
        int opcode = -1;
        int mod_b2 = -1;
        int reg_b3 = -1;
        int rm_b3 = -1;
        int d_b1 = -1;
        int w_b1 = -1;
        int displo_b8 = -1;
        int disphi_b8 = -1;
        int imlo_b8 = -1;
        int imhi_b8 = -1;
        int addrlo_b8 = -1;
        int addrhi_b8 = -1;

        int offset_begin = 0;
        int offset_end = 0;
    };


    static int get_disp_sz(int mod_b2, int rm_b3)
    {
        if (mod_b2 == 0b00 && rm_b3 == 0b110)
        {
            return 2;
        }

        switch (mod_b2)
        {
        case 0b00: return 0;
        case 0b01: return 1;
        case 0b10: return 2;
        case 0b11: return 0;
        }

        return 0;
    }


    static int get_w_sz(int w_b1)
    {
        if (w_b1)
        {
            return 2;
        }

        return 1;
    }


    static int get_disp_val(u8* disp, int disp_sz)
    {
        switch (disp_sz)
        {
        case 1: return (int)(*disp);            
        case 2: return (int)(*disp) + (*(disp + 1) << 8);            
        }

        return 0;
    }


    static void set_disp(InstrData& in, u8* disp, int disp_sz)
    {
        in.displo_b8 = *disp;

        if (disp_sz == 2)
        {
            in.disphi_b8 = *(disp + 1) << 8;
        }
    }


    static void set_im_data(InstrData& in, u8* im, int im_sz)
    {
        in.imlo_b8 = *im;

        if (im_sz == 2)
        {
            in.imhi_b8 = *(im + 1) << 8;
        }
    }


    static void set_addr(InstrData& in, u8* addr, int addr_sz)
    {
        in.addrlo_b8 = *addr;

        if (addr_sz == 2)
        {
            in.imhi_b8 = *(addr + 1) << 8;
        }
    }


    static InstrData get_rm_r(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        in.opcode = byte1 >> 2;
        in.d_b1 = (byte1 & 0b0000'0010) >> 1;
        in.w_b1 = byte1 & 0b0000'0001;
        in.mod_b2 = byte2 >> 6;
        in.reg_b3 = (byte2 & 0b00'111'000) >> 3;
        in.rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(in.mod_b2, in.rm_b3);
        set_disp(in, data + 2, disp_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 2 + disp_sz;

        return in;
    }


    static InstrData get_im_rm(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;
        in.mod_b2 = byte2 >> 6;
        in.rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(in.mod_b2, in.rm_b3);
        set_disp(in, data + 2, disp_sz);
        auto im_sz = get_w_sz(in.w_b1);
        set_im_data(in, data + 2 + disp_sz, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 2 + disp_sz + im_sz;

        return in;
    }


    static InstrData get_mov_im_r(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1 >> 4;
        in.w_b1 = (byte1 & 0b0000'1000) >> 3;
        in.reg_b3 = byte1 & 0b0000'0111;

        auto im_sz = get_w_sz(in.w_b1);
        set_im_data(in, data + 1, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + im_sz;

        return in;
    }


    static InstrData get_mov_m_ac(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;

        auto addr_sz = get_w_sz(in.w_b1);
        set_addr(in, data + 1, addr_sz);

        in.offset_begin = offset;
        in.offset_end = offset + addr_sz;

        return in;
    }


    static InstrData get_im_ac(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;

        auto im_sz = get_w_sz(in.w_b1);
        set_im_data(in, data + 1, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + im_sz;

        return in;
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


static void decode_bin_file(cstr bin_file)
{
    auto buffer = Bytes::read(bin_file);

    assert(buffer.data);
    assert(buffer.size);


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

    eval_file(file_044);

    printf("\nFinal registers:\n");
    REG::print_all();
}
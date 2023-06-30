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


namespace REG
{
    constexpr int HI_8 = 0b1111'1111'0000'0000;
    constexpr int LOW_8 = 0b0000'0000'1111'1111;
    constexpr int ZF = 0b0000'0000'0000'0001;
    constexpr int SF = 0b0000'0000'0000'0010;
    constexpr int PF = 0b0000'0000'0000'0100;
    constexpr int CF = 0b0000'0000'0000'1000;

    static u16 AX = 0;
    static u16 BX = 0;
    static u16 CX = 0;
    static u16 DX = 0;
    static u16 SP = 0;
    static u16 BP = 0;
    static u16 SI = 0;
    static u16 DI = 0;
    static u16 IP = 0;

    static u16 FLAGS = 0;

    static u8 MEM[1024 * 1024] = { 0 };

    static char trace_reg[20] = { 0 };
    static char trace_ip[20] = { 0 };
    static char trace_flags[20] = { 0 };


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

    static int ip() { return (int)IP; }

    static int zf() { return FLAGS & ZF; }


    static cstr get_flags_str()
    {
        switch (FLAGS)
        {
        case 0: return " ";
        case ZF: return "Z";
        case SF: return "S";
        case PF: return "P";
        case ZF | SF: return "ZS";
        case ZF | PF: return "PZ";
        case SF | PF: return "PS";
        case ZF | SF | PF: return "PZS"; 
        }

        return " ";
    }


    static void set_flags(u16 reg)
    {
        auto old = get_flags_str();

        FLAGS = 0;

        if (reg == 0)
        {
            FLAGS |= ZF;
            return;
        }

        if (reg & 0b1000'0000'0000'0000)
        {
            FLAGS |= SF;
        }

        memset(trace_flags, 0, sizeof(trace_flags));
        snprintf(trace_flags, sizeof(trace_flags), "flags:%s->%s", old, get_flags_str());
    }


    static void set_p_flag(u16 diff) 
    {
        auto old = get_flags_str();

        if (!diff)
        {
            FLAGS != PF;
        }

        memset(trace_flags, 0, sizeof(trace_flags));
        snprintf(trace_flags, sizeof(trace_flags), "flags:%s->%s", old, get_flags_str());
    }


    static void print_trace()
    {
        printf(" ; %s", trace_reg);
        printf(" %s", trace_ip);
        printf(" %s", trace_flags);
    }


    enum class Reg : int
    {
        al,
        bl,
        cl,
        dl,

        ah,
        bh,
        ch,
        dh,

        ax,
        bx,
        cx,
        dx,
        
        sp,
        bp,
        si,
        di,

        ip,

        none = -1
    };


    Reg get_reg(int reg_b3, int w)
    {
        using R = REG::Reg;

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


    static cstr reg_str(Reg r)
    {
        using R = REG::Reg;

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

        case R::ip: return "ip";
        }

        return "err";
    }


    static int get_value(Reg r)
    {
        using R = REG::Reg;

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


    static void set_reg_value(u16& reg, Reg name, int v)
    {
        auto old = reg;
        reg = (u16)v;
        memset(trace_reg, 0, sizeof(trace_reg));
        snprintf(trace_reg, sizeof(trace_reg), "%s:0x%x->0x%x", reg_str(name), old, reg);

        set_flags(reg);
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
        print("ip", ip());

        printf("flags: %s\n", get_flags_str());
    }
    
    
    static void set_ip(int v)
    {
        int old = IP;
        IP = (u16)v;
        memset(trace_ip, 0, sizeof(trace_ip));
        snprintf(trace_ip, sizeof(trace_ip), "ip:0x%x->0x%x", old, v);
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
        IP = 0;

        memset(MEM, 0, sizeof(MEM));
    }
}


// old

namespace REG
{
    enum class Name : int
    {
        m_bx_si,
        m_bx_di,
        m_bp_si,
        m_bp_di,
        m_si,
        m_di,
        m_direct_address,
        m_bx,

        m_bx_si_d8,
        m_bx_di_d8,
        m_bp_si_d8,
        m_bp_di_d8,
        m_si_d8,
        m_di_d8,
        m_bp_d8,
        m_bx_d8,

        m_bx_si_d16,
        m_bx_di_d16,
        m_bp_si_d16,
        m_bp_di_d16,
        m_si_d16,
        m_di_d16,
        m_bp_d16,
        m_bx_d16,        

        al,
        bl,
        cl,
        dl,

        ah,
        bh,
        ch,
        dh,

        ax,
        bx,
        cx,
        dx,
        
        sp,
        bp,
        si,
        di,

        ip,

        none = -1
    };


    Name get_reg2(int reg_b3, int w)
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


    Name get_reg_mem(int mod_b2, int rm_b3, int w_b1)
    {
        if (mod_b2 == 0b11)
        {
            return get_reg2(rm_b3, w_b1);
        }

        return (Name)(mod_b2 * 8 + rm_b3);        
    }


    static cstr decode(Name r)
    {
        using R = REG::Name;

        assert((int)r >= (int)R::al);

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

        case R::ip: return "ip";
        }

        return "err";
    }


    static void print_decode(Name r, int disp, char* str)
    {
        using R = REG::Name;

        if ((int)r >= (int)R::al)
        {
            snprintf(str, 10, "%s", decode(r));
            return;
        }

        if (r == R::m_direct_address)
        {            
            snprintf(str, 10, "%d", disp);
            return;
        }

        switch (r)
        {
        case R::m_bx_si:
            snprintf(str, 10, "bx + si");
            return;
        case R::m_bx_di:
            snprintf(str, 10, "bx + di");
            return;
        case R::m_bp_si:
            snprintf(str, 10, "bp + si");
            return;
        case R::m_bp_di:
            snprintf(str, 10, "bp + di");
            return;
        case R::m_si:
            snprintf(str, 10, "si");
            return;
        case R::m_di:
            snprintf(str, 10, "di");
            return;
        case R::m_bx:
            snprintf(str, 10, "bx");
            return;
        
        case R::m_bx_si_d8:
        case R::m_bx_si_d16:
            snprintf(str, 20, "bx + si + %d", disp);
        case R::m_bx_di_d8:
        case R::m_bx_di_d16:
            snprintf(str, 20, "bx + di + %d", disp);
        case R::m_bp_si_d8:
        case R::m_bp_si_d16:
            snprintf(str, 20, "bp + si + %d", disp);
        case R::m_bp_di_d8:
        case R::m_bp_di_d16:
            snprintf(str, 20, "bp + di + %d", disp);
        case R::m_si_d8:
        case R::m_si_d16:
            snprintf(str, 20, "si + %d", disp);
        case R::m_di_d8:
        case R::m_di_d16:
            snprintf(str, 20, "di + %d", disp);
        case R::m_bp_d8:
        case R::m_bp_d16:
            snprintf(str, 20, "bp + %d", disp);
        case R::m_bx_d8:
        case R::m_bx_d16:
            snprintf(str, 20, "bx + %d", disp);
        }

        snprintf(str, 10, "err");
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


    static void set_reg(u16& reg, Name name, int v)
    {
        auto old = reg;
        reg = (u16)v;
        memset(trace_reg, 0, sizeof(trace_reg));
        snprintf(trace_reg, sizeof(trace_reg), "%s:0x%x->0x%x", decode(name), old, reg);

        set_flags(reg);
    }

}


namespace DATA
{
    class InstrData
    {
    public:
        int opcode = -1;
        int mod_b2 = -1;
        int reg_b3 = -1;
        int rm_b3 = -1;
        int d_b1 = -1;
        int w_b1 = -1;
        int s_b1 = -1;
        int displo_b8 = -1;
        int disphi_b8 = -1;
        int imlo_b8 = -1;
        int imhi_b8 = -1;
        int addrlo_b8 = -1;
        int addrhi_b8 = -1;
        int j_b8 = -1;

        int disp_sz = 0;
        int im_sz = 0;
        int addr_sz = 0;

        int offset_begin = 0;
        int offset_end = 0;
    };


    void print_binary(u8 value) 
    {
        printf("[");
        for (int i = 7; i >= 4; --i) 
        {
            printf("%d", (value >> i) & 1);
        }

        printf(" ");

        for (int i = 3; i >= 0; --i) 
        {
            printf("%d", (value >> i) & 1);
        }
        printf("]");
    }


    static void print_binary(u8* data, InstrData const& inst)
    {
        for (int o = inst.offset_begin; o < inst.offset_end; ++o)
        {
            print_binary(data[o]);
        }

        printf(" ");
    }


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
        in.disp_sz = disp_sz;
        in.displo_b8 = *disp;

        if (disp_sz == 2)
        {
            in.disphi_b8 = *(disp + 1);
        }
    }


    static void set_im_data(InstrData& in, u8* im, int im_sz)
    {
        in.im_sz = im_sz;
        in.imlo_b8 = *im;

        if (im_sz == 2)
        {
            in.imhi_b8 = *(im + 1);
        }
    }


    static void set_addr(InstrData& in, u8* addr, int addr_sz)
    {
        in.addr_sz = addr_sz;
        in.addrlo_b8 = *addr;

        if (addr_sz == 2)
        {
            in.imhi_b8 = *(addr + 1);
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
        if (disp_sz)
        {
            set_disp(in, data + offset + 2, disp_sz);
        }        

        in.offset_begin = offset;
        in.offset_end = offset + 2 + disp_sz;

        REG::set_ip(in.offset_end);

        //print_binary(data, in);

        return in;
    }


    static InstrData get_im_rm(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;
        in.s_b1 = (byte1 & 0b0000'0010) >> 1;
        in.mod_b2 = byte2 >> 6;
        in.rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(in.mod_b2, in.rm_b3);
        if (disp_sz)
        {
            set_disp(in, data + offset + 2, disp_sz);
        }

        auto im_sz = get_w_sz(in.w_b1 && !in.s_b1);
        set_im_data(in, data + offset + 2 + disp_sz, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 2 + disp_sz + im_sz;

        REG::set_ip(in.offset_end);

        //print_binary(data, in);

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
        set_im_data(in, data + offset + 1, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 1 + im_sz;

        REG::set_ip(in.offset_end);

        //print_binary(data, in);

        return in;
    }


    static InstrData get_mov_m_ac(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;

        auto addr_sz = get_w_sz(in.w_b1);
        set_addr(in, data + offset + 1, addr_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 1 + addr_sz;

        REG::set_ip(in.offset_end);

        //print_binary(data, in);

        return in;
    }


    static InstrData get_im_ac(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;

        auto im_sz = get_w_sz(in.w_b1);
        set_im_data(in, data + offset + 1, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 1 + im_sz;

        REG::set_ip(in.offset_end);

        //print_binary(data, in);

        return in;
    }


    static InstrData get_jump(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];

        in.opcode = byte1;

        in.j_b8 = data[offset + 1];

        in.offset_begin = offset;
        in.offset_end = offset + 2;

        return in;
    }
}


namespace CMD
{
    using RM = REG::Name;

    using Reg = REG::Reg;


    class Im2Reg
    {
    public:
        int src = -1;
        Reg dst = Reg::none;        
    };


    class Reg2Reg
    {
    public:
        Reg src = Reg::none;
        Reg dst = Reg::none;
    };


    class RegMemReg
    {
    public:
        RM src = RM::none;
        RM dst = RM::none;

        int disp = -1;
    };


    class ImRegMem
    {
    public:
        int src = -1;

        int disp = -1;
    };


    class MemAcc
    {
    public:
        int src = -1;
        RM dst = RM::ax;
    };


    class AccMem
    {
    public:
        RM src = RM::ax;
        int dst = -1;        
    };


    class ImAcc
    {
    public:
        int src = -1;
        RM dst = RM::ax;
    };


    class Jump
    {
    public:
        int j_offset = 0;
    };


    static void print(RegMemReg const& cmd, cstr op)
    {
        char src[20] = { 0 };
        char dst[20] = { 0 };

        REG::print_decode(cmd.src, cmd.disp, src);
        REG::print_decode(cmd.dst, cmd.disp, dst);

        printf("%s %s, %s", op, dst, src);
    }


    static void print(Im2Reg const& cmd, cstr op)
    {
        auto src = cmd.src;
        printf("%s %s, %d", op, REG::reg_str(cmd.dst), src);
    }


    static void print(Reg2Reg const& cmd, cstr op)
    {
        printf("%s %s, %s", op, REG::reg_str(cmd.dst), REG::reg_str(cmd.src));
    }


    static void print(MemAcc const& cmd, cstr op)
    {
        auto src = cmd.src;
        auto dst = REG::decode(cmd.dst);        
        printf("%s %s, [%d]", op, dst, src);
    }


    static void print(AccMem const& cmd, cstr op)
    {
        auto src = REG::decode(cmd.src);
        auto dst = cmd.dst;
        printf("%s [%d], %s", op, dst, src);
    }


    static void print(ImAcc const& cmd, cstr op)
    {
        auto src = cmd.src;
        auto dst = REG::decode(cmd.dst);
        printf("%s %s, %d", op, dst, src);
    }


    static void print(Jump const& j, cstr op)
    {
        printf("%s $%d", op, j.j_offset);
    }


    static RegMemReg get_rm_r(DATA::InstrData const& in_data)
    {
        RegMemReg res{};

        /*auto r = REG::get_reg(in_data.reg_b3, in_data.w_b1);
        auto rm = REG::get_reg_mem(in_data.mod_b2, in_data.rm_b3, in_data.w_b1);

        if (in_data.d_b1)
        {
            res.dst = r;
            res.src = rm;
        }
        else
        {
            res.dst = rm;
            res.src = r;
        }

        if (in_data.disp_sz == 1)
        {
            res.disp = in_data.displo_b8;
        }
        else if (in_data.disp_sz == 2)
        {
            res.disp = in_data.displo_b8 + (in_data.disphi_b8 << 8);
        }*/

        return res;
    }


    static bool is_r2r(DATA::InstrData const& in_data)
    {
        return 
            in_data.opcode == 0b100010 &&
            in_data.mod_b2 == 0b11;
    }


    static bool is_rm2r(DATA::InstrData const& in_data)
    {
        return false;
    }


    static bool is_r2rm(DATA::InstrData const& in_data)
    {
        return false;
    }


    static Im2Reg get_im_rm(DATA::InstrData const& in_data)
    {
        Im2Reg res{};

        /*res.dst = REG::get_reg_mem(in_data.mod_b2, in_data.rm_b3, in_data.w_b1);

        if (in_data.disp_sz == 1)
        {
            res.disp = in_data.displo_b8;
        }
        else if (in_data.disp_sz == 2)
        {
            res.disp = in_data.displo_b8 + (in_data.disphi_b8 << 8);
        }

        if (in_data.im_sz == 1 || in_data.s_b1 == 1)
        {
            res.src = in_data.imlo_b8;
        }
        else if (in_data.im_sz == 2)
        {
            res.src = in_data.imlo_b8 + (in_data.imhi_b8 << 8);
        }*/

        return res;
    }


    static Im2Reg get_im_r(DATA::InstrData const& in_data)
    {
        Im2Reg res{};

        res.dst = REG::get_reg(in_data.reg_b3, in_data.w_b1);

        if (in_data.im_sz == 1)
        {
            res.src = in_data.imlo_b8;
        }
        else if (in_data.im_sz == 2)
        {
            res.src = in_data.imlo_b8 + (in_data.imhi_b8 << 8);
        }

        return res;
    }


    static Reg2Reg get_r_r(DATA::InstrData const& in_data)
    {
        Reg2Reg res{};

        auto reg = REG::get_reg(in_data.reg_b3, in_data.w_b1);
        auto rm = REG::get_reg(in_data.rm_b3, in_data.w_b1);

        if (in_data.d_b1)
        {
            res.src = rm;
            res.dst = reg;
        }
        else
        {
            res.src = reg;
            res.dst = rm;
        }

        return res;
    }


    static MemAcc get_m_ac(DATA::InstrData const& in_data)
    {
        MemAcc res{};

        if (in_data.addr_sz == 1)
        {
            res.src = in_data.addrlo_b8;
        }
        else if (in_data.addr_sz == 2)
        {
            res.src = in_data.addrlo_b8 + (in_data.addrhi_b8 << 8);
        }

        return res;
    }


    static AccMem get_ac_m(DATA::InstrData const& in_data)
    {
        AccMem res{};

        if (in_data.addr_sz == 1)
        {
            res.dst = in_data.addrlo_b8;
        }
        else if (in_data.addr_sz == 2)
        {
            res.dst = in_data.addrlo_b8 + (in_data.addrhi_b8 << 8);
        }

        return res;
    }


    static ImAcc get_im_ac(DATA::InstrData const& in_data)
    {
        ImAcc res{};

        if (in_data.im_sz == 1)
        {
            res.src = in_data.imlo_b8;
        }
        else if (in_data.im_sz == 2)
        {
            res.src = in_data.imlo_b8 + (in_data.imhi_b8 << 8);
        }

        return res;
    }


    static Jump get_jump(DATA::InstrData const& in_data)
    {
        Jump res{};

        auto j_b8 = in_data.j_b8;

        res.j_offset = *((i8*)(&j_b8)) + in_data.offset_end - in_data.offset_begin;

        return res;
    }
}


namespace MOV
{
    using R = REG::Reg;

    typedef void (*func_t)(int);


    static u16 set_high(u16 reg, int v)
    {
        return (u16)(((v & REG::LOW_8) << 8) + (reg & REG::LOW_8));
    }


    static u16 set_low(u16 reg, int v)
    {
        return (u16)((reg & REG::HI_8) + (v & REG::LOW_8));
    }


    static void ax(int v) { REG::set_reg_value(REG::AX, R::ax, v); }
    static void bx(int v) { REG::set_reg_value(REG::BX, R::bx, v); }
    static void cx(int v) { REG::set_reg_value(REG::CX, R::cx, v); }
    static void dx(int v) { REG::set_reg_value(REG::DX, R::dx, v); }

    static void ah(int v) { REG::set_reg_value(REG::AX, R::ax, set_high(REG::AX, v)); }
    static void bh(int v) { REG::set_reg_value(REG::BX, R::bx, set_high(REG::BX, v)); }
    static void ch(int v) { REG::set_reg_value(REG::DX, R::cx, set_high(REG::CX, v)); }
    static void dh(int v) { REG::set_reg_value(REG::DX, R::dx, set_high(REG::DX, v)); }

    static void al(int v) { REG::set_reg_value(REG::AX, R::ax, set_low(REG::AX, v)); }
    static void bl(int v) { REG::set_reg_value(REG::BX, R::bx, set_low(REG::BX, v)); }
    static void cl(int v) { REG::set_reg_value(REG::DX, R::cx, set_low(REG::CX, v)); }
    static void dl(int v) { REG::set_reg_value(REG::DX, R::dx, set_low(REG::DX, v)); }

    static void sp(int v) { REG::set_reg_value(REG::SP, R::sp, v); }
    static void bp(int v) { REG::set_reg_value(REG::BP, R::bp, v); }
    static void si(int v) { REG::set_reg_value(REG::SI, R::si, v); }
    static void di(int v) { REG::set_reg_value(REG::DI, R::di, v); }

    static void no_op(int) { printf("no op"); }


    static func_t get_mov_f(R reg)
    {
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


    static void rm_r(CMD::RegMemReg const& cmd)
    {
        CMD::print(cmd, "mov");

        /*auto f = get_mov_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }*/
    }


    static void r_r(CMD::Reg2Reg const& cmd)
    {
        CMD::print(cmd, "mov");

        auto f = get_mov_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void rm_r(DATA::InstrData const& in_data)
    {
        if (CMD::is_r2r(in_data))
        {
            auto cmd = CMD::get_r_r(in_data);
            r_r(cmd);
        }
        else if (CMD::is_r2rm(in_data))
        {

        }
        else if (CMD::is_rm2r(in_data))
        {

        }
        else
        {
            printf("X rm_r");
        }
    }


    static void im_r(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "mov");

        auto f = get_mov_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_rm2(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "mov");

        /*auto f = get_mov_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }*/
    }


    static void m_ac(CMD::MemAcc const& cmd)
    {
        CMD::print(cmd, "mov");
    }


    static void ac_m(CMD::AccMem const& cmd)
    {
        CMD::print(cmd, "mov");
    }
}


namespace ADD
{
    using R = REG::Name;

    typedef void (*func_t)(int);


    static u16 add_high(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        high += (u16)((v & REG::LOW_8) << 8);

        reg = high + low;
        REG::set_flags(reg);

        return reg;
    }


    static u16 add_low(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        low += (u16)((v & REG::LOW_8) << 8);        

        reg = high + low;
        REG::set_flags(reg);

        return reg;
    }


    static void ax(int v) { REG::set_reg(REG::AX, R::ax, REG::AX + v); }
    static void bx(int v) { REG::set_reg(REG::BX, R::bx, REG::BX + v); }
    static void cx(int v) { REG::set_reg(REG::CX, R::cx, REG::CX + v); }
    static void dx(int v) { REG::set_reg(REG::DX, R::dx, REG::DX + v); }

    static void ah(int v) { REG::set_reg(REG::AX, R::ax, add_high(REG::AX, v)); }
    static void bh(int v) { REG::set_reg(REG::BX, R::bx, add_high(REG::BX, v)); }
    static void ch(int v) { REG::set_reg(REG::CX, R::cx, add_high(REG::CX, v)); }
    static void dh(int v) { REG::set_reg(REG::DX, R::dx, add_high(REG::DX, v)); }

    static void al(int v) { REG::set_reg(REG::AX, R::ax, add_low(REG::AX, v)); }
    static void bl(int v) { REG::set_reg(REG::BX, R::bx, add_low(REG::BX, v)); }
    static void cl(int v) { REG::set_reg(REG::CX, R::cx, add_low(REG::CX, v)); }
    static void dl(int v) { REG::set_reg(REG::CX, R::cx, add_low(REG::DX, v)); }

    static void sp(int v) { REG::set_reg(REG::SP, R::sp, REG::SP + v); }
    static void bp(int v) { REG::set_reg(REG::BP, R::bp, REG::BP + v); }
    static void si(int v) { REG::set_reg(REG::SI, R::si, REG::SI + v); }
    static void di(int v) { REG::set_reg(REG::DI, R::di, REG::DI + v); }

    static void no_op(int) {}


    static func_t get_add_f(R reg)
    {
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


    static void rm_r(CMD::RegMemReg const& cmd)
    {
        CMD::print(cmd, "add");

        auto f = get_add_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_rm(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "add");

        /*auto f = get_add_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }*/
    }


    static void im_ac(CMD::ImAcc const& cmd)
    {
        CMD::print(cmd, "add");

        auto f = get_add_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }
}


namespace SUB
{
    using R = REG::Name;

    typedef void (*func_t)(int);


    static u16 sub_high(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        high -= (u16)((v & REG::LOW_8) << 8);

        reg = high + low;
        REG::set_flags(reg);

        return reg;
    }


    static u16 sub_low(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        low -= (u16)((v & REG::LOW_8) << 8);        

        reg = high + low;
        REG::set_flags(reg);

        return reg;
    }


    static void ax(int v) { REG::set_reg(REG::AX, R::ax, REG::AX - v); }
    static void bx(int v) { REG::set_reg(REG::BX, R::bx, REG::BX - v); }
    static void cx(int v) { REG::set_reg(REG::CX, R::cx, REG::CX - v); }
    static void dx(int v) { REG::set_reg(REG::DX, R::dx, REG::DX - v); }

    static void ah(int v) { REG::set_reg(REG::AX, R::ax, sub_high(REG::AX, v)); }
    static void bh(int v) { REG::set_reg(REG::BX, R::bx, sub_high(REG::BX, v)); }
    static void ch(int v) { REG::set_reg(REG::CX, R::cx, sub_high(REG::CX, v)); }
    static void dh(int v) { REG::set_reg(REG::DX, R::dx, sub_high(REG::DX, v)); }

    static void al(int v) { REG::set_reg(REG::AX, R::ax, sub_low(REG::AX, v)); }
    static void bl(int v) { REG::set_reg(REG::BX, R::bx, sub_low(REG::BX, v)); }
    static void cl(int v) { REG::set_reg(REG::CX, R::cx, sub_low(REG::CX, v)); }
    static void dl(int v) { REG::set_reg(REG::CX, R::cx, sub_low(REG::DX, v)); }

    static void sp(int v) { REG::set_reg(REG::SP, R::sp, REG::SP - v); }
    static void bp(int v) { REG::set_reg(REG::BP, R::bp, REG::BP - v); }
    static void si(int v) { REG::set_reg(REG::SI, R::si, REG::SI - v); }
    static void di(int v) { REG::set_reg(REG::DI, R::di, REG::DI - v); }

    static void no_op(int) {}


    static func_t get_sub_f(R reg)
    {
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


    static void rm_r(CMD::RegMemReg const& cmd)
    {
        CMD::print(cmd, "sub");

        auto f = get_sub_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_rm(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "sub");

        /*auto f = get_sub_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }*/
    }


    static void im_ac(CMD::ImAcc const& cmd)
    {
        CMD::print(cmd, "sub");

        auto f = get_sub_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }
}


namespace CMP
{
    using R = REG::Name;

    typedef void (*func_t)(int);


    static u16 cmp_high(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;

        return high - (u16)((v & REG::LOW_8) << 8);
    }


    static u16 cmp_low(u16 reg, int v)
    {
        auto low = (u16)(reg & REG::LOW_8);

        return low - (u16)((v & REG::LOW_8) << 8); 
    }


    static void ax(int v) { REG::set_p_flag(REG::AX - (u16)v); }
    static void bx(int v) { REG::set_p_flag(REG::BX - (u16)v); }
    static void cx(int v) { REG::set_p_flag(REG::CX - (u16)v); }
    static void dx(int v) { REG::set_p_flag(REG::DX - (u16)v); }

    static void ah(int v) { REG::set_p_flag(cmp_high(REG::AX, v)); }
    static void bh(int v) { REG::set_p_flag(cmp_high(REG::BX, v)); }
    static void ch(int v) { REG::set_p_flag(cmp_high(REG::CX, v)); }
    static void dh(int v) { REG::set_p_flag(cmp_high(REG::DX, v)); }

    static void al(int v) { REG::set_p_flag(cmp_low(REG::AX, v)); }
    static void bl(int v) { REG::set_p_flag(cmp_low(REG::BX, v)); }
    static void cl(int v) { REG::set_p_flag(cmp_low(REG::CX, v)); }
    static void dl(int v) { REG::set_p_flag(cmp_low(REG::DX, v)); }

    static void sp(int v) { REG::set_p_flag(REG::SP - (u16)v); }
    static void bp(int v) { REG::set_p_flag(REG::BP - (u16)v); }
    static void si(int v) { REG::set_p_flag(REG::SI - (u16)v); }
    static void di(int v) { REG::set_p_flag(REG::DI - (u16)v); }

    static void no_op(int) {}


    static func_t get_cmp_f(R reg)
    {
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


    static void rm_r(CMD::RegMemReg const& cmd)
    {
        CMD::print(cmd, "cmp");

        auto f = get_cmp_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_rm(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "cmp");

        /*auto f = get_cmp_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }*/
    }


    static void im_ac(CMD::ImAcc const& cmd)
    {
        CMD::print(cmd, "cmp");

        auto f = get_cmp_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }
}


namespace JUMP
{
    static void jnz(CMD::Jump const& cmd)
    {
        CMD::print(cmd, "jnz");
        if (REG::zf())
        {
            REG::set_ip(REG::ip() + 2);
        }
        else
        {
            REG::set_ip(REG::ip() + cmd.j_offset);
        }
    }
}


static int decode_next(u8* data, int offset)
{
    auto byte1 = data[offset];
    auto byte2 = data[offset + 1];        

    auto byte1_top4 = byte1 >> 4;
    auto byte1_top6 = byte1 >> 2;
    auto byte1_top7 = byte1 >> 1;

    auto byte2_345 = (byte2 & 0b00'111'000) >> 3;

    if (byte1_top6 == 0b0010'0010)
    {
        auto inst = DATA::get_rm_r(data, offset);
        MOV::rm_r(inst);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0110'0011)
    {
        auto inst = DATA::get_im_rm(data, offset);
        auto cmd = CMD::get_im_rm(inst);
        //MOV::im_rm(cmd);
        offset = REG::ip();
    }
    else if (byte1_top4 == 0b0000'1011)
    {
        auto inst = DATA::get_mov_im_r(data, offset);
        auto cmd = CMD::get_im_r(inst);
        MOV::im_r(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0110'0011)
    {
        auto inst = DATA::get_mov_m_ac(data, offset);
        auto cmd = CMD::get_m_ac(inst);
        MOV::m_ac(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0101'0001)
    {
        auto inst = DATA::get_mov_m_ac(data, offset);
        auto cmd = CMD::get_ac_m(inst);
        MOV::ac_m(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'0000)
    {
        auto inst = DATA::get_rm_r(data, offset);
        auto cmd = CMD::get_rm_r(inst);
        ADD::rm_r(cmd);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0000)
    {
        auto inst = DATA::get_im_rm(data, offset);
        auto cmd = CMD::get_im_rm(inst);
        ADD::im_rm(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'0110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        auto cmd = CMD::get_im_ac(inst);
        ADD::im_ac(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'1010)
    {
        auto inst = DATA::get_rm_r(data, offset);
        auto cmd = CMD::get_rm_r(inst);
        SUB::rm_r(cmd);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0101)
    {
        auto inst = DATA::get_im_rm(data, offset);
        auto cmd = CMD::get_im_rm(inst);
        SUB::im_rm(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'0110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        auto cmd = CMD::get_im_ac(inst);
        SUB::im_ac(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'1110)
    {
        auto inst = DATA::get_rm_r(data, offset);
        auto cmd = CMD::get_rm_r(inst);
        CMP::rm_r(cmd);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0111)
    {
        auto inst = DATA::get_im_rm(data, offset);
        auto cmd = CMD::get_im_rm(inst);
        CMP::im_rm(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'1110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        auto cmd = CMD::get_im_ac(inst);
        CMP::im_ac(cmd);
        offset = REG::ip();
    }

    else if (byte1 == 0b0111'0101)
    {
        auto inst = DATA::get_jump(data, offset);
        auto cmd = CMD::get_jump(inst);
        JUMP::jnz(cmd);
        offset = REG::ip();
    }

    else
    {
        offset = -1;
    }

    REG::print_trace();
    printf("\n");

    return offset;
}


static void decode_bin_file(cstr bin_file)
{
    auto buffer = Bytes::read(bin_file);

    assert(buffer.data);
    assert(buffer.size);

    int offset = 0;
    while (offset >= 0 && offset < buffer.size)
    {
        offset = decode_next(buffer.data, offset);
    }
}


int main()
{
    constexpr auto file_old = "../06/listing_0044_register_movs";
    constexpr auto file_051 = "listing_0051_memory_mov";
    constexpr auto file_052 = "listing_0052_memory_add_loop";

    decode_bin_file(file_old);

    printf("\nFinal registers:\n");
    REG::print_all();

    /*REG::reset();

    decode_bin_file(file_052);
    printf("\nFinal registers:\n");
    REG::print_all();*/
    
}
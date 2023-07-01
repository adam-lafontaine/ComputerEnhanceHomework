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

    static u16 ERR = 0;

    static u16 FLAGS = 0;

    static u8 MEM[100000] = { 0 };

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
        }
        else if (reg & 0b1000'0000'0000'0000)
        {
            FLAGS |= SF;
        }
        
        snprintf(trace_flags, sizeof(trace_flags), "flags:%s->%s", old, get_flags_str());
    }


    static void set_z_flag(u16 diff) 
    {
        auto old = get_flags_str();

        if (!diff)
        {
            FLAGS |= ZF;
        }

        snprintf(trace_flags, sizeof(trace_flags), "flags:%s->%s", old, get_flags_str());
    }


    static void print_trace()
    {
        if (strlen(trace_reg))
        {
            printf(" ; %s", trace_reg);
            memset(trace_reg, 0, sizeof(trace_reg));
        }

        if (strlen(trace_ip))
        {
            printf(" %s", trace_ip);
            memset(trace_ip, 0, sizeof(trace_ip));
        }

        if (strlen(trace_flags))
        {
            printf(" %s", trace_flags);
            memset(trace_flags, 0, sizeof(trace_flags));
        }
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


    static cstr get_str(Reg r)
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


    static u16& get_ref(Reg r)
    {
        using R = REG::Reg;

        switch (r)
        {
        case R::ax: return AX;
        case R::bx: return BX;
        case R::cx: return CX;
        case R::dx: return DX;

        case R::sp: return SP;
        case R::bp: return BP;
        case R::si: return SI;
        case R::di: return DI;
        }

        return ERR;
    }


    static void mov_reg_value(u16& reg, Reg name, int v)
    {
        auto old = reg;
        reg = (u16)v;        
        snprintf(trace_reg, sizeof(trace_reg), "%s:0x%x->0x%x", get_str(name), old, reg);
    }


    static void set_reg_value(u16& reg, Reg name, int v)
    {
        mov_reg_value(reg, name, v);
        set_flags(reg);
    }


    static void set_reg_value(Reg name, int v)
    {
        set_reg_value(get_ref(name), name, v);
    }
    

    enum class MemReg : int
    {
        m_bx_si,
        m_bx_di,
        m_bp_si,
        m_bp_di,
        m_si,
        m_di,
        m_bp,
        m_bx,

        none = -1
    };


    static cstr get_str(MemReg mr)
    {
        switch(mr)
        {
        case MemReg::m_bx_si: return "bx + si";
        case MemReg::m_bx_di: return "bx + di";
        case MemReg::m_bp_si: return "bp + si";
        case MemReg::m_bp_di: return "bp + di";
        case MemReg::m_si: return "si";
        case MemReg::m_di: return "di";
        case MemReg::m_bp: return "bp";
        case MemReg::m_bx: return "bx";
        }

        return "err";
    }


    static MemReg get_mem_reg(int rm_b3, int mod_b2)
    {
        if(rm_b3 == 0b110 && mod_b2 == 0b00)
        {
            return MemReg::none;
        }

        return (MemReg)rm_b3;
    }


    static int get_value(MemReg mr)
    {
        switch(mr)
        {
        case MemReg::m_bx_si: return BX + SI;
        case MemReg::m_bx_di: return BX + DI;
        case MemReg::m_bp_si: return BP + SI;
        case MemReg::m_bp_di: return BP + DI;
        case MemReg::m_si: return SI;
        case MemReg::m_di: return DI;
        case MemReg::m_bp: return BP;
        case MemReg::m_bx: return BX;
        }

        return -1;
    }

    
}


namespace DATA
{
    class ByteArray
    {
    public:
        u8* data = 0;
        u32 length = 0;
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


    static void print(ByteArray const& arr)
    {
        for (u32 i = 0; i < arr.length; ++i)
        {
            print_binary(arr.data[i]);
        }

        printf(" ");
    }


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

        ByteArray bytes;
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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

        return in;
    }


    static InstrData get_im_rm(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        in.opcode = byte1 >> 2;
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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

        return in;
    }


    static InstrData get_mov_im_rm(u8* data, int offset)
    {
        InstrData in{};   

        auto byte1 = data[offset];
        auto byte2 = data[offset + 1];

        in.opcode = byte1 >> 1;
        in.w_b1 = byte1 & 0b0000'0001;
        in.mod_b2 = byte2 >> 6;
        in.rm_b3 = byte2 & 0b00'000'111;

        auto disp_sz = get_disp_sz(in.mod_b2, in.rm_b3);
        if (disp_sz)
        {
            set_disp(in, data + offset + 2, disp_sz);
        }

        auto im_sz = get_w_sz(in.w_b1);
        set_im_data(in, data + offset + 2 + disp_sz, im_sz);

        in.offset_begin = offset;
        in.offset_end = offset + 2 + disp_sz + im_sz;

        REG::set_ip(in.offset_end);

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

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

        in.bytes.data = data + offset;
        in.bytes.length = in.offset_end - in.offset_begin;

        return in;
    }
}


namespace CMD
{
    using Reg = REG::Reg;
    using MR = REG::MemReg;


    class Im2Reg
    {
    public:
        int src = -1;
        Reg dst = Reg::none;        
    };


    static void print(Im2Reg const& cmd, cstr op)
    {
        printf("%s %s, %d", op, REG::get_str(cmd.dst), cmd.src);
    }


    static Im2Reg get_im_r(DATA::InstrData const& in_data)
    {
        Im2Reg res{};

        res.dst = REG::get_reg(in_data.rm_b3, in_data.w_b1);

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


    static Im2Reg get_mov_im_r(DATA::InstrData const& in_data)
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


    class Reg2Reg
    {
    public:
        Reg src = Reg::none;
        Reg dst = Reg::none;
    };


    static void print(Reg2Reg const& cmd, cstr op)
    {
        printf("%s %s, %s", op, REG::get_str(cmd.dst), REG::get_str(cmd.src));
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


    class Mem2Reg
    {
    public:
        int src = -1;
        Reg dst = Reg::none;
    };


    static void print(Mem2Reg const& cmd, cstr op)
    {
        printf("%s word %s, [%d]", op, REG::get_str(cmd.dst), cmd.src);
    }


    static Mem2Reg get_m_r(DATA::InstrData const& in_data)
    {
        Mem2Reg res{};

        res.dst = REG::get_reg(in_data.reg_b3, in_data.w_b1);

        assert(in_data.disp_sz == 2);
        
        res.src = in_data.displo_b8 + (in_data.disphi_b8 << 8);

        return res;
    }


    class Reg2MemReg
    {
    public:
        Reg src = Reg::none;
        MR dst = MR::none;
    };


    static void print(Reg2MemReg const& cmd, cstr op)
    {
        printf("%s word [%s], %s", op, REG::get_str(cmd.dst), REG::get_str(cmd.src));
    }


    static Reg2MemReg get_r_mr(DATA::InstrData const& in_data)
    {
        Reg2MemReg res{};

        res.src = REG::get_reg(in_data.reg_b3, in_data.w_b1);
        res.dst = REG::get_mem_reg(in_data.rm_b3, in_data.mod_b2);

        return res;
    }


    class RegMem2Reg
    {
    public:
        MR src = MR::none;
        Reg dst = Reg::none;
    };


    static void print(RegMem2Reg const& cmd, cstr op)
    {
        printf("%s %s, [%s]", op, REG::get_str(cmd.dst), REG::get_str(cmd.src));
    }


    static RegMem2Reg get_rm_r(DATA::InstrData const& in_data)
    {
        RegMem2Reg res{};

        res.src = REG::get_mem_reg(in_data.rm_b3, in_data.mod_b2);
        res.dst = REG::get_reg(in_data.reg_b3, in_data.w_b1);

        return res;
    }


    class Im2Mem
    {
    public:
        int src = -1;
        int dst = -1;

        int im_size = -1;
    };


    static void print(Im2Mem const& cmd, cstr op)
    {
        auto sz = "err";
        if (cmd.im_size == 1)
        {
            sz = "byte";
        }
        else if (cmd.im_size == 2)
        {
            sz = "word";
        }

        printf("%s %s [%d], %d", op, sz, cmd.dst, cmd.src);     
    }


    static Im2Mem get_im_m(DATA::InstrData const& in_data)
    {
        Im2Mem res{};

        assert(in_data.disp_sz == 2);

        res.im_size = in_data.im_sz;
        res.dst = in_data.displo_b8 + (in_data.disphi_b8 << 8);

        if (in_data.im_sz == 1 || in_data.s_b1 == 1)
        {
            res.src = in_data.imlo_b8;
        }
        else if (in_data.im_sz == 2)
        {
            res.src = in_data.imlo_b8 + (in_data.imhi_b8 << 8);
        }

        return res;
    }


    class Im2MemRegDisp
    {
    public:
        int src = -1;
        MR dst = MR::none;

        int im_size = -1;
        int disp = -1;
    };


    static void print(Im2MemRegDisp const& cmd, cstr op)
    {
        auto sz = "err";
        if (cmd.im_size == 1)
        {
            sz = "byte";
        }
        else if (cmd.im_size == 2)
        {
            sz = "word";
        }

        printf("%s %s [%s + %d], %d", op, sz, REG::get_str(cmd.dst), cmd.disp, cmd.src);
    }


    static Im2MemRegDisp get_im_rmd(DATA::InstrData const& in_data)
    {
        Im2MemRegDisp res{};

        res.dst = REG::get_mem_reg(in_data.rm_b3, in_data.mod_b2);

        res.im_size = in_data.im_sz;
        
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
        }

        //DATA::print(in_data.bytes);

        return res;
    }


    class Jump
    {
    public:
        int j_offset = 0;
    };
   

    static void print(Jump const& j, cstr op)
    {
        printf("%s $%d", op, j.j_offset);
    }


    static bool is_r_r(DATA::InstrData const& in_data)
    {
        return 
            in_data.mod_b2 == 0b11;
    }


    static bool is_m_r(DATA::InstrData const& in_data)
    {
        return
            in_data.d_b1 == 1 &&
            in_data.mod_b2 == 0b00 &&
            in_data.rm_b3 == 0b110;
    }


    static bool is_rm_r(DATA::InstrData const& in_data)
    {
        return 
            in_data.d_b1 == 1 &&
            in_data.mod_b2 != 0b11;
    }


    static bool is_r_rm(DATA::InstrData const& in_data)
    {
        return 
            in_data.d_b1 == 0 &&
            in_data.mod_b2 != 0b11;

    }


    static bool is_im_r(DATA::InstrData const& in_data)
    {
        return
            in_data.opcode == 0b100000 &&
            in_data.mod_b2 == 0b11 &&
            (in_data.im_sz == 1 || in_data.im_sz == 2);
    }


    static bool is_m(DATA::InstrData const& in_data)
    {
        return
            in_data.mod_b2 == 0b00 &&
            in_data.rm_b3 == 0b110;
    }


    static bool is_rmd(DATA::InstrData const& in_data)
    {
        return
            (in_data.mod_b2 == 0b01 || in_data.mod_b2 == 0b10);
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


    static void ax(int v) { REG::mov_reg_value(REG::AX, R::ax, v); }
    static void bx(int v) { REG::mov_reg_value(REG::BX, R::bx, v); }
    static void cx(int v) { REG::mov_reg_value(REG::CX, R::cx, v); }
    static void dx(int v) { REG::mov_reg_value(REG::DX, R::dx, v); }

    static void ah(int v) { REG::mov_reg_value(REG::AX, R::ax, set_high(REG::AX, v)); }
    static void bh(int v) { REG::mov_reg_value(REG::BX, R::bx, set_high(REG::BX, v)); }
    static void ch(int v) { REG::mov_reg_value(REG::DX, R::cx, set_high(REG::CX, v)); }
    static void dh(int v) { REG::mov_reg_value(REG::DX, R::dx, set_high(REG::DX, v)); }

    static void al(int v) { REG::mov_reg_value(REG::AX, R::ax, set_low(REG::AX, v)); }
    static void bl(int v) { REG::mov_reg_value(REG::BX, R::bx, set_low(REG::BX, v)); }
    static void cl(int v) { REG::mov_reg_value(REG::DX, R::cx, set_low(REG::CX, v)); }
    static void dl(int v) { REG::mov_reg_value(REG::DX, R::dx, set_low(REG::DX, v)); }

    static void sp(int v) { REG::mov_reg_value(REG::SP, R::sp, v); }
    static void bp(int v) { REG::mov_reg_value(REG::BP, R::bp, v); }
    static void si(int v) { REG::mov_reg_value(REG::SI, R::si, v); }
    static void di(int v) { REG::mov_reg_value(REG::DI, R::di, v); }

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


    static void mov_r_r(CMD::Reg2Reg const& cmd)
    {
        CMD::print(cmd, "mov");

        auto f = get_mov_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void mov_m_r(CMD::Mem2Reg const& cmd)
    {
        print(cmd, "mov");

        auto p16 = (u16*)(REG::MEM + cmd.src);
        REG::set_reg_value(cmd.dst, *p16);
    }


    static void mov_r_rm(CMD::Reg2MemReg const& cmd)
    {
        print(cmd, "mov");

        auto p16 = (u16*)(REG::MEM + REG::get_value(cmd.dst));
        *p16 = REG::get_value(cmd.src);
    }


    static void mov_rm_r(CMD::RegMem2Reg const& cmd)
    {
        print(cmd, "mov");

        auto p16 = (u16*)(REG::MEM + REG::get_value(cmd.src));
        REG::set_reg_value(cmd.dst, *p16);
    }


    static void mov_im_m(CMD::Im2Mem const& cmd)
    {
        CMD::print(cmd, "mov");

        auto p8 = REG::MEM + cmd.dst;

        if (cmd.im_size == 1)
        {
            *p8 = (u8)cmd.src;
        }
        else if (cmd.im_size == 2)
        {
            auto p16 = (u16*)p8;
            *p16 = (u16)cmd.src;
        }
    }


    static void mov_im_rmd(CMD::Im2MemRegDisp const& cmd)
    {
        CMD::print(cmd, "mov");

        auto p8 = REG::MEM + REG::get_value(cmd.dst) + cmd.disp;

        if (cmd.im_size == 1)
        {
            *p8 = (u8)cmd.src;
        }
        else if (cmd.im_size == 2)
        {
            auto p16 = (u16*)p8;
            *p16 = (u16)cmd.src;
        }
    }


    static void rm_r(DATA::InstrData const& in_data)
    {
        if (CMD::is_r_r(in_data))
        {
            auto cmd = CMD::get_r_r(in_data);
            mov_r_r(cmd);
        }
        else if (CMD::is_m_r(in_data))
        {            
            auto cmd = CMD::get_m_r(in_data);
            mov_m_r(cmd);
        }
        else if (CMD::is_r_rm(in_data))
        {
            auto cmd = CMD::get_r_mr(in_data);
            mov_r_rm(cmd);
        }
        else if (CMD::is_rm_r(in_data))
        {
            auto cmd = CMD::get_rm_r(in_data);
            mov_rm_r(cmd);
        }
        else
        {
            
            printf("X rm_r");
        }
    }


    static void im_rm(DATA::InstrData const& in_data)
    {
        if (CMD::is_m(in_data))
        {
            auto cmd = CMD::get_im_m(in_data);
            mov_im_m(cmd);
        }
        else if (CMD::is_rmd(in_data))
        {
            auto cmd = CMD::get_im_rmd(in_data);
            mov_im_rmd(cmd);
        }
        else
        {
            printf("X im_rm");
        }
    }


    static void im_r(DATA::InstrData const& in_data)
    {
        auto cmd = CMD::get_mov_im_r(in_data);
        CMD::print(cmd, "mov");

        auto f = get_mov_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }
}


namespace ADD
{
    using R = REG::Reg;

    typedef void (*func_t)(int);


    static u16 add_high(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        high += (u16)((v & REG::LOW_8) << 8);

        reg = high + low;

        return reg;
    }


    static u16 add_low(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        low += (u16)((v & REG::LOW_8) << 8);        

        reg = high + low;

        return reg;
    }


    static void ax(int v) { REG::set_reg_value(REG::AX, R::ax, REG::AX + v); }
    static void bx(int v) { REG::set_reg_value(REG::BX, R::bx, REG::BX + v); }
    static void cx(int v) { REG::set_reg_value(REG::CX, R::cx, REG::CX + v); }
    static void dx(int v) { REG::set_reg_value(REG::DX, R::dx, REG::DX + v); }

    static void ah(int v) { REG::set_reg_value(REG::AX, R::ax, add_high(REG::AX, v)); }
    static void bh(int v) { REG::set_reg_value(REG::BX, R::bx, add_high(REG::BX, v)); }
    static void ch(int v) { REG::set_reg_value(REG::CX, R::cx, add_high(REG::CX, v)); }
    static void dh(int v) { REG::set_reg_value(REG::DX, R::dx, add_high(REG::DX, v)); }

    static void al(int v) { REG::set_reg_value(REG::AX, R::ax, add_low(REG::AX, v)); }
    static void bl(int v) { REG::set_reg_value(REG::BX, R::bx, add_low(REG::BX, v)); }
    static void cl(int v) { REG::set_reg_value(REG::CX, R::cx, add_low(REG::CX, v)); }
    static void dl(int v) { REG::set_reg_value(REG::CX, R::cx, add_low(REG::DX, v)); }

    static void sp(int v) { REG::set_reg_value(REG::SP, R::sp, REG::SP + v); }
    static void bp(int v) { REG::set_reg_value(REG::BP, R::bp, REG::BP + v); }
    static void si(int v) { REG::set_reg_value(REG::SI, R::si, REG::SI + v); }
    static void di(int v) { REG::set_reg_value(REG::DI, R::di, REG::DI + v); }

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


    static void add_im_r(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "add");

        auto f = get_add_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }


    static void add_r_r(CMD::Reg2Reg const& cmd)
    {
        CMD::print(cmd, "add");

        auto f = get_add_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_rm(DATA::InstrData const& in_data)
    {
        if (CMD::is_im_r(in_data))
        {
            auto cmd = CMD::get_im_r(in_data);
            add_im_r(cmd);
        }
        else
        {
            DATA::print(in_data.bytes);
        }
    }


    static void rm_r(DATA::InstrData const& in_data)
    {
        if (CMD::is_r_r(in_data))
        {
            auto cmd = CMD::get_r_r(in_data);
            add_r_r(cmd);
        }
        else
        {
            DATA::print(in_data.bytes);
        }
    }
}


namespace SUB
{
    using R = REG::Reg;

    typedef void (*func_t)(int);


    static u16 sub_high(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        high -= (u16)((v & REG::LOW_8) << 8);

        reg = high + low;

        return reg;
    }


    static u16 sub_low(u16 reg, int v)
    {
        auto high = (u16)(reg & REG::HI_8) << 8;
        auto low = (u16)(reg & REG::LOW_8);

        low -= (u16)((v & REG::LOW_8) << 8);        

        reg = high + low;

        return reg;
    }


    static void ax(int v) { REG::set_reg_value(REG::AX, R::ax, REG::AX - v); }
    static void bx(int v) { REG::set_reg_value(REG::BX, R::bx, REG::BX - v); }
    static void cx(int v) { REG::set_reg_value(REG::CX, R::cx, REG::CX - v); }
    static void dx(int v) { REG::set_reg_value(REG::DX, R::dx, REG::DX - v); }

    static void ah(int v) { REG::set_reg_value(REG::AX, R::ax, sub_high(REG::AX, v)); }
    static void bh(int v) { REG::set_reg_value(REG::BX, R::bx, sub_high(REG::BX, v)); }
    static void ch(int v) { REG::set_reg_value(REG::CX, R::cx, sub_high(REG::CX, v)); }
    static void dh(int v) { REG::set_reg_value(REG::DX, R::dx, sub_high(REG::DX, v)); }

    static void al(int v) { REG::set_reg_value(REG::AX, R::ax, sub_low(REG::AX, v)); }
    static void bl(int v) { REG::set_reg_value(REG::BX, R::bx, sub_low(REG::BX, v)); }
    static void cl(int v) { REG::set_reg_value(REG::CX, R::cx, sub_low(REG::CX, v)); }
    static void dl(int v) { REG::set_reg_value(REG::CX, R::cx, sub_low(REG::DX, v)); }

    static void sp(int v) { REG::set_reg_value(REG::SP, R::sp, REG::SP - v); }
    static void bp(int v) { REG::set_reg_value(REG::BP, R::bp, REG::BP - v); }
    static void si(int v) { REG::set_reg_value(REG::SI, R::si, REG::SI - v); }
    static void di(int v) { REG::set_reg_value(REG::DI, R::di, REG::DI - v); }

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


    static void r_r(CMD::Reg2Reg const& cmd)
    {
        CMD::print(cmd, "sub");

        auto f = get_sub_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void im_r(CMD::Im2Reg const& cmd)
    {
        CMD::print(cmd, "sub");

        auto f = get_sub_f(cmd.dst);
        auto val = cmd.src;
        if (val >= 0)
        {
            f(val);
        }
    }


    static void rm_r(DATA::InstrData const& in_data)
    {
        if (CMD::is_r_r(in_data))
        {
            auto cmd = CMD::get_r_r(in_data);
            r_r(cmd);
        }
        else
        {
            printf("X rm_r");
        }
    }


    static void im_rm(DATA::InstrData const& in_data)
    {
        if (CMD::is_im_r(in_data))
        {
            auto cmd = CMD::get_im_r(in_data);
            im_r(cmd);
        }
        else
        {
            printf("X im_r");
        }
    }
}


namespace CMP
{
    using R = REG::Reg;

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


    static void ax(int v) { REG::set_z_flag(REG::AX - (u16)v); }
    static void bx(int v) { REG::set_z_flag(REG::BX - (u16)v); }
    static void cx(int v) { REG::set_z_flag(REG::CX - (u16)v); }
    static void dx(int v) { REG::set_z_flag(REG::DX - (u16)v); }

    static void ah(int v) { REG::set_z_flag(cmp_high(REG::AX, v)); }
    static void bh(int v) { REG::set_z_flag(cmp_high(REG::BX, v)); }
    static void ch(int v) { REG::set_z_flag(cmp_high(REG::CX, v)); }
    static void dh(int v) { REG::set_z_flag(cmp_high(REG::DX, v)); }

    static void al(int v) { REG::set_z_flag(cmp_low(REG::AX, v)); }
    static void bl(int v) { REG::set_z_flag(cmp_low(REG::BX, v)); }
    static void cl(int v) { REG::set_z_flag(cmp_low(REG::CX, v)); }
    static void dl(int v) { REG::set_z_flag(cmp_low(REG::DX, v)); }

    static void sp(int v) { REG::set_z_flag(REG::SP - (u16)v); }
    static void bp(int v) { REG::set_z_flag(REG::BP - (u16)v); }
    static void si(int v) { REG::set_z_flag(REG::SI - (u16)v); }
    static void di(int v) { REG::set_z_flag(REG::DI - (u16)v); }

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


    static void r_r(CMD::Reg2Reg const& cmd)
    {
        CMD::print(cmd, "cmp");

        auto f = get_cmp_f(cmd.dst);
        auto val = REG::get_value(cmd.src);
        if (val >= 0)
        {
            f(val);
        }
    }


    static void rm_r(DATA::InstrData const& in_data)
    {
        if (CMD::is_r_r(in_data))
        {
            auto cmd = CMD::get_r_r(in_data);
            r_r(cmd);
        }
        else
        {
            printf("X rm_r");
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
        auto inst = DATA::get_mov_im_rm(data, offset);
        MOV::im_rm(inst);
        offset = REG::ip();
    }
    else if (byte1_top4 == 0b0000'1011)
    {
        auto inst = DATA::get_mov_im_r(data, offset);
        MOV::im_r(inst);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0110'0011)
    {
        auto inst = DATA::get_mov_m_ac(data, offset);
        //auto cmd = CMD::get_m_ac(inst);
        //MOV::m_ac(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0101'0001)
    {
        auto inst = DATA::get_mov_m_ac(data, offset);
        //auto cmd = CMD::get_ac_m(inst);
        //MOV::ac_m(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'0000)
    {
        auto inst = DATA::get_rm_r(data, offset);
        ADD::rm_r(inst);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0000)
    {
        auto inst = DATA::get_im_rm(data, offset);
        ADD::im_rm(inst);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'0110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        //auto cmd = CMD::get_im_ac(inst);
        //ADD::im_ac(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'1010)
    {
        auto inst = DATA::get_rm_r(data, offset);
        SUB::rm_r(inst);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0101)
    {
        auto inst = DATA::get_im_rm(data, offset);
        SUB::im_rm(inst);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'0110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        //auto cmd = CMD::get_im_ac(inst);
        //SUB::im_ac(cmd);
        offset = REG::ip();
    }

    else if (byte1_top6 == 0b0000'1110)
    {
        auto inst = DATA::get_rm_r(data, offset);
        CMP::rm_r(inst);
        offset = REG::ip();
    }
    else if (byte1_top6 == 0b0010'0000 && byte2_345 == 0b0000'0111)
    {
        auto inst = DATA::get_im_rm(data, offset);
        //auto cmd = CMD::get_im_rm(inst);
        //CMP::im_rm(cmd);
        offset = REG::ip();
    }
    else if (byte1_top7 == 0b0001'1110)
    {
        auto inst = DATA::get_im_ac(data, offset);
        //auto cmd = CMD::get_im_ac(inst);
        //CMP::im_ac(cmd);
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
    //constexpr auto file_old = "../06/listing_0044_register_movs";
    //constexpr auto file_old = "../07/listing_0046_add_sub_cmp";
    //constexpr auto file_old = "../08/listing_0048_ip_register";
    //constexpr auto file_old = "../08/listing_0049_conditional_jumps";

    constexpr auto file_051 = "listing_0051_memory_mov";
    constexpr auto file_052 = "listing_0052_memory_add_loop";

    decode_bin_file(file_052);

    printf("\nFinal registers:\n");
    REG::print_all();

    /*REG::reset();

    decode_bin_file(file_052);
    printf("\nFinal registers:\n");
    REG::print_all();*/
    
}
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
        printf("%s %s %s\n", inst.op.str, inst.dst.str, inst.src.str);
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
            }
        }

        Bytes::destroy(buffer);
    }
}


int main()
{
    ASM::print_file("listing_0043_immediate_movs.asm");


}
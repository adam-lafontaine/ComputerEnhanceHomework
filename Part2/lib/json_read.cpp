#include <cstring>


namespace
{
    class State
    {
    public:
        b32 is_open = false;

        char* data = nullptr;

        f64 x0 = 0.0;
        f64 y0 = 0.0;
        f64 x1 = 0.0;
        f64 y1 = 0.0;

        f64 total = 0.0;
        u32 count = 0;

        cstr error = 0;
    };
}


static int update(State& state, int offset)
{
    ++offset;
    if (!state.is_open)
    {        
        return offset;
    }

    ++state.count;
    state.total += haversine_earth(state.x0, state.y0, state.x1, state.y1);

    state.x0 = 0.0;
    state.y0 = 0.0;
    state.x1 = 0.0;
    state.y1 = 0.0;

    return offset;
}


static int json_key_value(State& state, int offset)
{
    auto data = state.data;
    assert(data[offset] == '\"');

    ++offset;
    if (!state.is_open)
    {        
        return offset;
    }

    constexpr int key_x0 = 0b000;
    constexpr int key_y0 = 0b001;
    constexpr int key_x1 = 0b010;
    constexpr int key_y1 = 0b011;
    constexpr int err = 0b100;

    int key = 0;

    switch(data[offset])
    {
    case 'X':        
        break;

    case 'Y':
        key &= 0b001;
        break;

    default:
        key &= err;
    }

    ++offset;

    switch(data[offset])
    {
    case '0':
        break;

    case '1':
        key &= 0b010;
        break;

    default:
        key &= err;
    }

    ++offset;

    auto chars = "-.0123456789";
    while (std::strchr(chars, data[offset]) == nullptr)
    {
        ++offset;
    }

    char value_str[50] = { 0 };
    int i = 0;

    while (std::strchr(chars, data[offset]) != nullptr)
    {
        value_str[i++] = data[offset++];
    }

    auto end = value_str;
    f64 value = std::strtod(value_str, &end);

    if (*end != 0)
    {
        state.error = "parse f64 error";
        return -1;
    }

    switch(key)
    {
    case key_x0:
        state.x0 = value;
        break;

    case key_x1:
        state.x1 = value;
        break;

    case key_y0:
        state.y0 = value;
        break;

    case key_y1:
        state.y1 = value;
        break;

    default:
        state.error = "parse key error";
        return -1;
    }

    return offset;
}


static int process_next(State& state, int offset)
{
    auto c = state.data[offset];

    switch (c)
    {
    case '[':
        state.is_open = true;
        ++offset;
        break;

    case ']':
        state.is_open = false;
        ++offset;
        break;
        
    case '}':
        offset = update(state, offset);
        break;
        
    case '\"':
        offset = json_key_value(state, offset);
        break;
    
    default:
        ++offset;
    }

    return offset;
}


HavOut process_json(cstr json_path)
{
    auto buffer = mb::read_buffer<char>(json_path);

    State state{};

    int offset = 0;
    while (offset >= 0 && offset < buffer.size_)
    {
        offset = process_next(state, offset);
    }

    HavOut result{};
    result.input_size = buffer.size_;
    result.input_count = state.count;
    result.msg = "OK";
    result.avg = state.total / state.count;

    if (state.error)
    {
        result.error = true;
        result.msg = state.error;
    }

    return result;
}
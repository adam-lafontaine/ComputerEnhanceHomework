HavOut process_bin(cstr bin_path)
{
    HavOut result{};

    auto buffer = mb::read_buffer<f64>(bin_path);
    if (!buffer.data)
    {
        result.error = true;
        result.msg = "read error";
        return result;
    }

    f64 total = 0.0;
    int offset = 0;
    while (offset < buffer.size_)
    {
        total += buffer.data[offset++];
    }
    
    result.input_size = buffer.size_ * sizeof(f64);
    result.input_count = (u32)buffer.size_;
    result.avg = total / result.input_count;
    result.msg = "OK";

    return result;
}
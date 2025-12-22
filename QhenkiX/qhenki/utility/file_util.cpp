#include "qhenki/utility/file_util.h"

#include <cassert>
#include <fstream>
#include <locale>

#include <filesystem>

namespace qhenki::util
{
template<typename CharT> static bool read_file_impl(const CharT* path, void** data, size_t* size)
{
    assert(data);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return false;
    }

    std::streamsize stream_size = file.tellg();
    if (stream_size < 0)
    {
        return false;
    }
    *size = static_cast<size_t>(stream_size);
    file.seekg(0, std::ios::beg);

    *data = malloc(stream_size);

    if (!file.read(reinterpret_cast<char*>(*data), stream_size))
    {
        free(*data); // Free on failure
        *data = nullptr;
        return false;
    }

    return true;
}

template<typename CharT> static bool write_file_impl(const CharT* path, const void* data, size_t size)
{
    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data), size);
    if (!file.good())
    {
        return false;
    }

    return true;
}

bool read_file(const wchar_t* path, void** data, size_t* size)
{
    return read_file_impl(path, data, size);
}

bool write_file(const wchar_t* path, const void* data, const size_t size)
{
    return write_file_impl(path, data, size);
}

bool read_file(const char* path, void** data, size_t* size)
{
    return read_file_impl(path, data, size);
}

bool write_file(const char* path, const void* data, const size_t size)
{
    return write_file_impl(path, data, size);
}
} // namespace qhenki::util

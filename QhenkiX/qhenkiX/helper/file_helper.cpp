#include "qhenkiX/helper/file_helper.h"

#include <cassert>
#include <fstream>
#include <vector>
#include <locale>

#include <windows.h>
#include <filesystem>

using namespace qhenki::util;

template <typename CharT>
static bool read_file_impl(const CharT* path, void** data, size_t* size) 
{
    assert(data);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        if constexpr (std::is_same_v<CharT, wchar_t>) 
        {
            wchar_t debug_msg[512]{};
            swprintf(debug_msg, sizeof(debug_msg) / sizeof(wchar_t),
                L"FileHelper:: Failed to open file: %ls\n", path);
            OutputDebugStringW(debug_msg);
        }
        else 
        {
            char debug_msg[512]{};
            snprintf(debug_msg, sizeof(debug_msg),
                "FileHelper:: Failed to open file: %s\n", path);
            OutputDebugStringA(debug_msg);
        }
        return false;
    }

    std::streamsize stream_size = file.tellg();
    if (stream_size < 0)
    {
        OutputDebugString(L"FileHelper:: Failed to read stream");
        return false;
    }
	*size = static_cast<size_t>(stream_size);
    file.seekg(0, std::ios::beg);

    *data = malloc(stream_size);

    if (!file.read(reinterpret_cast<char*>(*data), stream_size)) 
    {
        if constexpr (std::is_same_v<CharT, wchar_t>) 
        {
            wchar_t debug_msg[512]{};
            swprintf(debug_msg, sizeof(debug_msg) / sizeof(wchar_t),
                L"FileHelper:: Failed to read file: %ls\n", path);
            OutputDebugStringW(debug_msg);
        }
        else
        {
            char debug_msg[512]{};
            snprintf(debug_msg, sizeof(debug_msg),
                "FileHelper:: Failed to read file: %s\n", path);
            OutputDebugStringA(debug_msg);
        }
		free(*data); // Free on failure
        *data = nullptr;
        return false;
    }

    return true;
}

template <typename CharT>
static bool write_file_impl(const CharT* path, const void* data, size_t size) 
{
    std::basic_ofstream<CharT> file(path, std::ios::binary);
    if (!file) 
    {
        if constexpr (std::is_same_v<CharT, wchar_t>) 
        {
            wchar_t debug_msg[512]{};
            swprintf(debug_msg, sizeof(debug_msg) / sizeof(wchar_t),
                L"FileHelper:: Failed to open file: %ls\n", path);
            OutputDebugStringW(debug_msg);
        }
        else 
        {
            char debug_msg[512]{};
            snprintf(debug_msg, sizeof(debug_msg),
                "FileHelper:: Failed to open file: %s\n", path);
            OutputDebugStringA(debug_msg);
        }
        return false;
    }

    file.write(reinterpret_cast<const CharT*>(data), size / sizeof(CharT));
    if (!file) 
    {
        if constexpr (std::is_same_v<CharT, wchar_t>) 
        {
            wchar_t debug_msg[512]{};
            swprintf(debug_msg, sizeof(debug_msg) / sizeof(wchar_t),
                L"FileHelper:: Failed to write file: %ls\n", path);
            OutputDebugStringW(debug_msg);
        }
        else
        {
            char debug_msg[512]{};
            snprintf(debug_msg, sizeof(debug_msg),
                "FileHelper:: Failed to write file: %s\n", path);
            OutputDebugStringA(debug_msg);
        }
        return false;
    }

    return true;
}

bool FileHelper::read_file(const wchar_t* path, void** data, size_t* size)
{
    return read_file_impl(path, data, size);
}

bool FileHelper::write_file(const wchar_t* path, const void* data, size_t size)
{
    return write_file_impl(path, data, size);
}

bool FileHelper::read_file(const char* path, void** data, size_t* size)
{
    return read_file_impl(path, data, size);
}

bool FileHelper::write_file(const char* path, const void* data, size_t size)
{
    return write_file_impl(path, data, size);
}

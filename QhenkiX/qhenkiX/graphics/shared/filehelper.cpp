#include "filehelper.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <locale>
#include <windows.h>

using namespace qhenki::util;

std::optional<std::vector<uint8_t>> FileHelper::read_file(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
		OutputDebugString((L"FileHelper:: Failed to open file: " + path + L"\n").c_str());
		return std::nullopt;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
		OutputDebugString((L"FileHelper:: Failed to read file: " + path + L"\n").c_str());
		return std::nullopt;
    }

    return buffer;
}

bool FileHelper::write_file(const std::wstring& path, const void* data, size_t size)
{
	std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        OutputDebugString((L"FileHelper:: Failed to open file: " + path + L"\n").c_str());
		return false;
    }
	file.write(reinterpret_cast<const char*>(data), size);
    if (!file)
    {
        OutputDebugString((L"FileHelper:: Failed to write file: " + path + L"\n").c_str());
		return false;
    }
    return true;
}

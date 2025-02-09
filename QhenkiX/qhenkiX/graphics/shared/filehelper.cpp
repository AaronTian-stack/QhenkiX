#include "filehelper.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <locale>

std::optional<std::vector<uint8_t>> FileHelper::read_file(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::wcerr << L"FileHelper:: Failed to open file: " << path << std::endl;
		return std::nullopt;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        std::wcerr << L"FileHelper:: Failed to read file: " << path << std::endl;
		return std::nullopt;
    }

    return buffer;
}

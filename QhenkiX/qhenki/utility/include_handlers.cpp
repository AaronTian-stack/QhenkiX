#include "qhenki/utility/include_handlers.h"

#include "qhenki/utility/string_util.h"

#include <cassert>
#include <filesystem>
#include <fstream>

HRESULT __stdcall qhenki::gfx::MultiIncludeHandler::Open(
    D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
    for (const auto& dir : m_include_paths)
    {
        if (!std::filesystem::is_directory(dir))
        {
            continue; // Skip if not valid directory
        }

        std::ifstream file;

        constexpr size_t path_size = 1024;
        if (dir.size() + strlen(pFileName) + 1 < path_size)
        {
            const auto full_path = qhenki::util::format_string<path_size>("%s/%s", dir.c_str(), pFileName);
            file.open(full_path.buffer.data(), std::ios::binary | std::ios::ate);
        }
        else
        {
            std::string full_path = dir + "/" + pFileName;
            file.open(full_path, std::ios::binary | std::ios::ate);
        }

        if (file)
        {
            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            char* buffer = new char[size];
            if (file.read(buffer, size))
            {
                *ppData = buffer;
                *pBytes = static_cast<UINT>(size);
                return S_OK;
            }
            delete[] buffer;
        }
    }
    return E_FAIL;
}

HRESULT __stdcall qhenki::gfx::MultiIncludeHandler::Close(LPCVOID pData)
{
    delete[] reinterpret_cast<const char*>(pData); // Open allocates memory with new char[]
    return S_OK;
}

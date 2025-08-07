#include "qhenkiX/utility/include_handlers.h"

#include <cassert>
#include <filesystem>
#include <fstream>

HRESULT __stdcall qhenki::gfx::MultiIncludeHandler::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
    for (const auto& dir : m_include_paths)
    {
        if (!std::filesystem::is_directory(dir))
        {
            continue; // Skip if not valid directory
        }

        std::ifstream file;

        char fullPath[1024];
        if (dir.size() + strlen(pFileName) + 1 < sizeof(fullPath))
        {
            std::snprintf(fullPath, sizeof(fullPath), "%s/%s", dir.c_str(), pFileName);
			file.open(fullPath, std::ios::binary | std::ios::ate);
        }
        else
        {
            std::string fullPath = dir + "/" + pFileName;
			file.open(fullPath, std::ios::binary | std::ios::ate);
        }

        if (file)
        {
            std::streamsize size = file.tellg();
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

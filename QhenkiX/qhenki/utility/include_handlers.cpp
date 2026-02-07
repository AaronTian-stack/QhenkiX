#include "qhenki/utility/include_handlers.h"
#include "qhenki/utility/file_util.h"

#include <filesystem>

namespace fs = std::filesystem;

qhenki::gfx::MultiIncludeHandler::MultiIncludeHandler(const std::span<const std::string> include_paths,
                                                      const std::string_view source_path)
    : m_include_paths(include_paths),
      m_source_path(source_path)
{
}

qhenki::gfx::MultiIncludeHandler::MultiIncludeHandler(const std::vector<std::string>& include_paths,
                                                      const std::string_view source_path)
    : m_include_paths(include_paths),
      m_source_path(source_path)
{
}

HRESULT __stdcall qhenki::gfx::MultiIncludeHandler::Open(
    D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
    void* data = nullptr;
    size_t size = 0;

    auto try_path = [&](const fs::path& path)
    {
        if (!util::read_file(path.string().c_str(), &data, &size))
        {
            return false;
        }
        *ppData = data;
        *pBytes = static_cast<UINT>(size);
        return true;
    };

    // Resolve relative to the source file's directory first
    if (!m_source_path.empty())
    {
        const fs::path source_dir = fs::path(m_source_path).parent_path();
        if ((source_dir.has_parent_path() || !source_dir.empty()) && try_path(source_dir / pFileName))
        {
            return S_OK;
        }
    }

    for (const auto& dir : m_include_paths)
    {
        if (!fs::is_directory(dir))
        {
            continue;
        }
        if (try_path(fs::path(dir) / pFileName))
        {
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT __stdcall qhenki::gfx::MultiIncludeHandler::Close(LPCVOID pData)
{
    free(const_cast<void*>(pData));
    return S_OK;
}

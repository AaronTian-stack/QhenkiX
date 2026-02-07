#include "dxc_include_handler.h"

#include <wrl/client.h>
#include <filesystem>

using Microsoft::WRL::ComPtr;

namespace fs = std::filesystem;

namespace qhenki::gfx
{
DxcIncludeHandlerForCompile::DxcIncludeHandlerForCompile(IDxcUtils* utils,
                                                         const std::string_view source_path,
                                                         const std::span<const std::string> include_paths)
    : m_source_path(source_path),
      m_include_paths(include_paths),
      m_utils(utils)
{
}

HRESULT STDMETHODCALLTYPE DxcIncludeHandlerForCompile::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
    {
        return E_POINTER;
    }
    if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DxcIncludeHandlerForCompile::AddRef()
{
    return ++m_refcount;
}

ULONG STDMETHODCALLTYPE DxcIncludeHandlerForCompile::Release()
{
    const ULONG r = --m_refcount;
    if (r == 0)
    {
        m_utils.Reset();
    }
    return r;
}

HRESULT STDMETHODCALLTYPE DxcIncludeHandlerForCompile::LoadSource(
    _In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource)
{
    if (!pFilename || !ppIncludeSource)
    {
        return E_INVALIDARG;
    }
    *ppIncludeSource = nullptr;

    const fs::path requested(pFilename);

    if (!m_source_path.empty())
    {
        const fs::path source_dir = fs::path(m_source_path).parent_path();
        const fs::path candidate = source_dir / requested;
        if (fs::exists(candidate))
        {
            return load_file(fs::absolute(candidate), ppIncludeSource);
        }
    }

    for (const auto& dir : m_include_paths)
    {
        if (!fs::is_directory(dir))
        {
            continue;
        }
        const fs::path candidate = fs::path(dir) / requested;
        if (fs::exists(candidate))
        {
            return load_file(fs::absolute(candidate), ppIncludeSource);
        }
    }

    return E_FAIL;
}

HRESULT DxcIncludeHandlerForCompile::load_file(const fs::path& path, IDxcBlob** ppIncludeSource) const
{
    const std::wstring path_w = path.wstring();
    ComPtr<IDxcBlobEncoding> blob;
    if (FAILED(m_utils->LoadFile(path_w.c_str(), nullptr, &blob)))
    {
        return E_FAIL;
    }
    *ppIncludeSource = blob.Detach();
    return S_OK;
}
} // namespace qhenki::gfx

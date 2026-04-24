#pragma once

#include "dxc_com_ptr.h"

#include <atomic>
#include <filesystem>
#include <span>
#include <string>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
// Default handler has no notion of the main file so this is needed when compiling from a buffer
class DxcIncludeHandlerForCompile final : public IDxcIncludeHandler
{
public:
    DxcIncludeHandlerForCompile(IDxcUtils* utils,
                                std::string_view source_path,
                                std::span<const std::string> include_paths);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename,
                                         _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override;
    HRESULT load_file(const std::filesystem::path& path, IDxcBlob** ppIncludeSource) const;
    ~DxcIncludeHandlerForCompile() = default;

private:
    std::string_view m_source_path;
    std::span<const std::string> m_include_paths;
    ComPtr<IDxcUtils> m_utils;
    std::atomic_uint32_t m_refcount{1};
};
} // namespace qhenki::gfx

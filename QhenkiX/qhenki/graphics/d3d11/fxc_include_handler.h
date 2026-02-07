#pragma once

#include <d3dcommon.h>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace qhenki::gfx
{
// Multi-include path include handler, mainly for FXC since you can't specify multiple paths in D3DCompileFromFile
class MultiIncludeHandler final : public ID3DInclude
{
    std::span<const std::string> m_include_paths;
    std::string_view m_source_path;

public:
    explicit MultiIncludeHandler(std::span<const std::string> include_paths, std::string_view source_path = {});
    explicit MultiIncludeHandler(const std::vector<std::string>& include_paths, std::string_view source_path = {});

    HRESULT STDMETHODCALLTYPE
    Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
    HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override;
};
}; // namespace qhenki::gfx

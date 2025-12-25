#pragma once

#include <d3dcommon.h>
#include <dxcapi.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include "qhenki/RHI/shader_compiler.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D12ShaderOutput
{
    ComPtr<IDxcBlob> shader_blob;
};

class D3D12ShaderCompiler : public ShaderCompiler
{
    ComPtr<IDxcUtils> m_library;
    ComPtr<IDxcCompiler3> m_compiler; // Not thread safe

    static DXGI_FORMAT mask_to_format(uint32_t mask, D3D_REGISTER_COMPONENT_TYPE type);

public:
    D3D12ShaderCompiler();

    static bool get_compiler_path(char* buffer, size_t length);
    bool get_compiler_path_v(char* buffer, size_t length) override;
    bool compile(const CompilerInput& input, CompilerOutput& output) override;

    ~D3D12ShaderCompiler() override = default;

    friend class D3D12Context;
};
} // namespace qhenki::gfx

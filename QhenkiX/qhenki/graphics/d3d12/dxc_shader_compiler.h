#pragma once

#include <directx/d3dcommon.h>
#include <directx/dxgiformat.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include "qhenki/RHI/shader_compiler.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class DXCShaderCompiler : public ShaderCompiler
{
    ComPtr<IDxcUtils> m_library;
    ComPtr<IDxcCompiler3> m_compiler; // Not thread safe

public:
    DXCShaderCompiler();

    static bool get_compiler_path(char* buffer, size_t length);
    bool compile(const CompilerInput& input, CompilerOutput& output, bool output_spirv = false) override;

    ~DXCShaderCompiler() override = default;

    friend class D3D12Context;

private:
    static DXGI_FORMAT mask_to_format(uint32_t mask, D3D_REGISTER_COMPONENT_TYPE type);
};
} // namespace qhenki::gfx

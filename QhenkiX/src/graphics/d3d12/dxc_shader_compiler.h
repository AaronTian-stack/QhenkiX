#pragma once

#include "dxc_com_ptr.h"

#include "qhenki/rhi/shader_compiler.h"

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
};
} // namespace qhenki::gfx

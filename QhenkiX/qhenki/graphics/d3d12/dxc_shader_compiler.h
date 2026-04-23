#pragma once

#include "dxc_com_ptr.h"

#include <directx/dxgiformat.h>
#if defined(_WIN32) || defined(_WIN64)
#include <directx/d3dcommon.h>
#else
// Clone of D3D_REGISTER_COMPONENT_TYPE so I don't have to include d3dcommon.h which would cause collisions with <dxc/WinAdapter.h>
// https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_register_component_type
typedef enum D3D_REGISTER_COMPONENT_TYPE
{
    D3D_REGISTER_COMPONENT_UNKNOWN = 0,
    D3D_REGISTER_COMPONENT_UINT32 = 1,
    D3D_REGISTER_COMPONENT_SINT32 = 2,
    D3D_REGISTER_COMPONENT_FLOAT32 = 3,
    D3D_REGISTER_COMPONENT_UINT16,
    D3D_REGISTER_COMPONENT_SINT16,
    D3D_REGISTER_COMPONENT_FLOAT16,
    D3D_REGISTER_COMPONENT_UINT64,
    D3D_REGISTER_COMPONENT_SINT64,
    D3D_REGISTER_COMPONENT_FLOAT64,
    D3D10_REGISTER_COMPONENT_UNKNOWN,
    D3D10_REGISTER_COMPONENT_UINT32,
    D3D10_REGISTER_COMPONENT_SINT32,
    D3D10_REGISTER_COMPONENT_FLOAT32,
    D3D10_REGISTER_COMPONENT_UINT16,
    D3D10_REGISTER_COMPONENT_SINT16,
    D3D10_REGISTER_COMPONENT_FLOAT16,
    D3D10_REGISTER_COMPONENT_UINT64,
    D3D10_REGISTER_COMPONENT_SINT64,
    D3D10_REGISTER_COMPONENT_FLOAT64
} D3D_REGISTER_COMPONENT_TYPE;
#endif

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

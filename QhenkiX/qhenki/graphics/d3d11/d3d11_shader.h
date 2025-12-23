#pragma once

#include <d3d11.h>
#include <d3dcommon.h>
#include <wrl/client.h>
#include <variant>

#include "qhenki/RHI/shader.h"
#include "qhenki/RHI/shader_compiler.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D11VertexShader
{
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3DBlob> vertex_shader_blob;
};

class D3D11Shader
{
    ShaderType m_type;
    std::variant<ComPtr<ID3D11PixelShader>, D3D11VertexShader> m_shader;

public:
    D3D11Shader(ID3D11Device* device,
                ShaderType shader_type,
                const void* data,
                size_t size,
                const char* dbg_name,
                bool* result);
    D3D11Shader(
        ID3D11Device* device, ShaderType shader_type, const char* name, const CompilerOutput& output, bool* result);

    friend class D3D11Context;
    friend struct D3D11GraphicsPipeline;
};
} // namespace qhenki::gfx

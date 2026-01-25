#include "d3d11_shader.h"

#include <d3dcompiler.h>
#include <codecvt>
#include <stdexcept>

#include "d3d11_context.h"
#include "qhenki/utility/d3d_util.h"

using namespace qhenki::gfx;

D3D11Shader::D3D11Shader(ID3D11Device* device,
                         const ShaderType shader_type,
                         const void* data,
                         const size_t size,
                         const char* dbg_name,
                         bool* result)
    : m_type(shader_type)
{
    ID3D11DeviceChild* device_resource = nullptr;
    switch (shader_type)
    {
    case VERTEX_SHADER:
    {
        m_shader = D3D11VertexShader();
        // Needs to keep a copy of the vertex shader for reflection purposes
        ComPtr<ID3DBlob> blob;
        if (D3DCreateBlob(size, &blob))
        {
            *result = false;
            break;
        }
        memcpy(blob->GetBufferPointer(), data, size);
        if (FAILED(device->CreateVertexShader(blob->GetBufferPointer(),
                                              blob->GetBufferSize(),
                                              nullptr,
                                              &std::get<D3D11VertexShader>(m_shader).vertex_shader)))
        {
            *result = false;
        }
        else
        {
            device_resource = std::get<D3D11VertexShader>(m_shader).vertex_shader.Get();
            auto& tvs = std::get<D3D11VertexShader>(m_shader);
            tvs.vertex_shader_blob = blob;
        }
        break;
    }
    case PIXEL_SHADER:
    {
        m_shader = ComPtr<ID3D11PixelShader>();
        if (FAILED(device->CreatePixelShader(
                data, size, nullptr, std::get<ComPtr<ID3D11PixelShader>>(m_shader).ReleaseAndGetAddressOf())))
        {
            *result = false;
        }
        else
        {
            device_resource = std::get<ComPtr<ID3D11PixelShader>>(m_shader).Get();
        }
        break;
    }
    default:
        throw std::runtime_error("D3D11: Shader type not implemented");
    }

    if (device_resource)
    {
        set_debug_name(device_resource, dbg_name);
    }

    *result = true;
}

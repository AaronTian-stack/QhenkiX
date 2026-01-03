#include "d3d11_layout_assembler.h"

#include "qhenki/utility/string_util.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

using namespace qhenki::gfx;

namespace
{
template<typename T> void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

std::size_t hash_input_element(const D3D11_INPUT_ELEMENT_DESC& desc)
{
    std::size_t hash = 0;
    hash_combine(hash, desc.SemanticName);
    hash_combine(hash, desc.SemanticIndex);
    hash_combine(hash, desc.Format);
    hash_combine(hash, desc.InputSlot);
    hash_combine(hash, desc.AlignedByteOffset);
    hash_combine(hash, desc.InputSlotClass);
    hash_combine(hash, desc.InstanceDataStepRate);
    return hash;
}

std::size_t hash_input_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout)
{
    std::size_t seed = 0;

    for (const auto& desc : layout)
    {
        hash_combine(seed, hash_input_element(desc));
    }

    return seed;
}
} // namespace

void D3D11LayoutAssembler::add_input(const D3D11_INPUT_ELEMENT_DESC& input)
{
    m_layout_desc.push_back(input);
}

ID3D11InputLayout* D3D11LayoutAssembler::find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout)
{
    const auto hash = hash_input_layout(layout);
    if (m_layout_map.contains(hash))
    {
        return m_layout_map[hash].layout.Get();
    }

    return nullptr;
}

D3D11Layout* D3D11LayoutAssembler::find_layout(ID3D11InputLayout* layout)
{
    std::scoped_lock lock(m_layout_mutex);
    if (m_layout_logical_map.contains(layout))
    {
        return m_layout_logical_map[layout];
    }
    return nullptr;
}

#define find_layout(layout_d)                \
    auto hash = hash_input_layout(layout_d); \
    if (m_layout_map.contains(hash))         \
        return m_layout_map[hash].layout.Get();


std::optional<ComPtr<ID3D11InputLayout>> D3D11LayoutAssembler::create_input_layout_manual(
    ID3D11Device* const device, ID3DBlob* const vertex_shader_blob)
{
    std::scoped_lock lock(m_layout_mutex);
    // Hash the input layout
    find_layout(m_layout_desc)

        ComPtr<ID3D11InputLayout>
            layout;
    if (FAILED(device->CreateInputLayout(m_layout_desc.data(),
                                         static_cast<UINT>(m_layout_desc.size()),
                                         vertex_shader_blob->GetBufferPointer(),
                                         vertex_shader_blob->GetBufferSize(),
                                         layout.ReleaseAndGetAddressOf())))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Input Layout manual\n");
        return {};
    }

    m_layout_map[hash] = {.layout = layout, .desc = m_layout_desc};
    m_layout_logical_map[layout.Get()] = &m_layout_map[hash];

    return layout;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> D3D11LayoutAssembler::create_input_layout_desc(
    ID3D11ShaderReflection* vs_reflection, const bool increment_slot)
{
    assert(vs_reflection);

    D3D11_SHADER_DESC shader_desc;
    if (FAILED(vs_reflection->GetDesc(&shader_desc)))
    {
        return {};
    }

    UINT slot = 0;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
    input_layout_desc.reserve(shader_desc.InputParameters);
    for (UINT i = 0; i < shader_desc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC param_desc;
        if (FAILED(vs_reflection->GetInputParameterDesc(i, &param_desc)))
        {
            continue;
        }

        // Ignore system attributes
        if (param_desc.SystemValueType == D3D_NAME_VERTEX_ID || param_desc.SystemValueType == D3D_NAME_PRIMITIVE_ID ||
            param_desc.SystemValueType == D3D_NAME_INSTANCE_ID)
        {
            continue;
        }

        D3D11_INPUT_ELEMENT_DESC element_desc = {
            .SemanticName = param_desc.SemanticName,
            .SemanticIndex = param_desc.SemanticIndex,
            .InputSlot = slot,
            .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            // TODO: INSTANCING
            .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        // Determine DXGI format
        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32_UINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32_SINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                element_desc.Format = DXGI_FORMAT_R32_FLOAT;
            }
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32_UINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32_SINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
            }
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            }
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
        else
        {
            const auto error_msg =
                qhenki::util::format_string<256>("D3D11: Unsupported input format for %s[%d] with mask %d\n",
                                                 param_desc.SemanticName,
                                                 param_desc.SemanticIndex,
                                                 param_desc.Mask);
            OutputDebugStringA(error_msg.buffer.data());
        }

        if (increment_slot)
        {
            slot++;
        }

        // save element desc
        input_layout_desc.push_back(element_desc);
    }

    return input_layout_desc;
}

ID3D11InputLayout* D3D11LayoutAssembler::create_input_layout_reflection(ID3D11Device* const device,
                                                                        ID3DBlob* const vertex_shader_blob,
                                                                        bool increment_slot)
{
    ComPtr<ID3D11ShaderReflection> vs_shader_reflection;
    if (FAILED(D3DReflect(vertex_shader_blob->GetBufferPointer(),
                          vertex_shader_blob->GetBufferSize(),
                          IID_ID3D11ShaderReflection,
                          &vs_shader_reflection)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Input layout reflection failed\n");
        return nullptr;
    }

    D3D11_SHADER_DESC shader_desc;
    if FAILED (vs_shader_reflection->GetDesc(&shader_desc))
    {
        return nullptr;
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc = create_input_layout_desc(vs_shader_reflection.Get(),
                                                                                       increment_slot);

    if (input_layout_desc.empty())
    {
        return nullptr;
    }

    // hash and check if layout already exists
    std::scoped_lock lock(m_layout_mutex);
    find_layout(input_layout_desc)

        ComPtr<ID3D11InputLayout>
            layout;
    if (FAILED(device->CreateInputLayout(input_layout_desc.data(),
                                         input_layout_desc.size(),
                                         vertex_shader_blob->GetBufferPointer(),
                                         vertex_shader_blob->GetBufferSize(),
                                         &layout)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Input Layout reflection\n");
        return nullptr;
    }

    m_layout_map[hash] = {.layout = layout, .desc = std::move(input_layout_desc)};
    m_layout_logical_map[layout.Get()] = &m_layout_map[hash];

    return layout.Get();
}

void D3D11LayoutAssembler::clear_maps()
{
    std::scoped_lock lock(m_layout_mutex);
    m_layout_map.clear();
    m_layout_logical_map.clear();
    m_layout_desc.clear();
}

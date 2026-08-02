#include "d3d11_layout_assembler.h"

#include <d3dcompiler.h>

#include "qhenki/utility/string_util.h"
#include "src/utility/d3d_reflection_util.h"

#include "qhenki/rhi/shader.h"

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


D3D11Layout* D3D11LayoutAssembler::find_layout(ID3D11InputLayout* layout)
{
    std::scoped_lock lock(m_layout_mutex);
    if (m_layout_logical_map.contains(layout))
    {
        return m_layout_logical_map[layout];
    }
    return nullptr;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> D3D11LayoutAssembler::create_input_layout_desc(
    ID3D11ShaderReflection* const vs_reflection,
    const D3D11_SHADER_DESC& shader_desc,
    const bool increment_slot,
    unsigned* const out_builtins)
{
    assert(vs_reflection && out_builtins);
    *out_builtins = 0;

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

        if (param_desc.SystemValueType != D3D_NAME_UNDEFINED)
        {
            ++*out_builtins;
            continue;
        }

        D3D11_INPUT_ELEMENT_DESC element_desc{
            .SemanticName = param_desc.SemanticName,
            .SemanticIndex = param_desc.SemanticIndex,
            .Format = mask_to_format(param_desc.Mask, param_desc.ComponentType),
            .InputSlot = slot,
            .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };
        if (element_desc.Format == DXGI_FORMAT_UNKNOWN)
        {
            const auto error_msg =
                util::format_string<256>("D3D11: Unsupported reflected input format for %s[%d] with mask %d\n",
                                         param_desc.SemanticName,
                                         param_desc.SemanticIndex,
                                         param_desc.Mask);
            OutputDebugStringA(error_msg.buffer.data());
            continue;
        }

        if (increment_slot)
        {
            ++slot;
        }
        input_layout_desc.push_back(element_desc);
    }

    return input_layout_desc;
}

InputLayoutResult D3D11LayoutAssembler::create_input_layout_reflection(ID3D11Device* const device,
                                                                       const Shader shader,
                                                                       const bool increment_slot,
                                                                       const char* const debug_name)
{
    ComPtr<ID3D11ShaderReflection> reflection;
    if (FAILED(D3DReflect(shader.data, shader.size, IID_ID3D11ShaderReflection, &reflection)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Native DXBC reflection failed\n");
        return {.layout = nullptr, .is_empty = false};
    }

    D3D11_SHADER_DESC shader_desc{};
    if (FAILED(reflection->GetDesc(&shader_desc)))
    {
        return {.layout = nullptr, .is_empty = false};
    }

    unsigned system_inputs = 0;
    auto input_layout_desc = create_input_layout_desc(reflection.Get(), shader_desc, increment_slot, &system_inputs);
    if (input_layout_desc.empty())
    {
        if (shader_desc.InputParameters == system_inputs)
        {
            return {.layout = nullptr, .is_empty = true};
        }
        return {.layout = nullptr, .is_empty = false};
    }
    if (input_layout_desc.size() + system_inputs != shader_desc.InputParameters)
    {
        return {.layout = nullptr, .is_empty = false};
    }

    std::scoped_lock lock(m_layout_mutex);

    const auto hash = hash_input_layout(input_layout_desc);
    if (m_layout_map.contains(hash))
    {
        return {.layout = m_layout_map[hash].layout.Get(), .is_empty = false};
    }

    ComPtr<ID3D11InputLayout> layout;
    const auto result = device->CreateInputLayout(
        input_layout_desc.data(), input_layout_desc.size(), shader.data, shader.size, &layout);
    if (FAILED(result))
    {
        const auto error_message =
            util::format_string<512>("Qhenki D3D11 ERROR: Failed to create input layout for '%s' (HRESULT 0x%08X)\n",
                                     debug_name ? debug_name : "<unnamed>",
                                     static_cast<unsigned>(result));
        OutputDebugStringA(error_message.buffer.data());
        return {.layout = nullptr, .is_empty = false};
    }

    m_layout_map[hash] = {.layout = layout, .desc = std::move(input_layout_desc)};
    m_layout_logical_map[layout.Get()] = &m_layout_map[hash];
    return {.layout = layout.Get(), .is_empty = false};
}

void D3D11LayoutAssembler::clear_maps()
{
    m_layout_map.clear();
    m_layout_logical_map.clear();
}

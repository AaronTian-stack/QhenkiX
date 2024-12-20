#include "layoutassembler.h"

template <typename T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

std::size_t hash_string(LPCSTR str)
{
    std::size_t hash = 0;
    while (*str)
    {
        hash_combine(hash, *str);
        str++;
    }
    return hash;
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

void LayoutAssembler::add_input(const D3D11_INPUT_ELEMENT_DESC& input)
{
    layout_desc.push_back(input);
}

ID3D11InputLayout* LayoutAssembler::create_input_layout(const ComPtr<ID3D11Device>& device,
    const ComPtr<ID3DBlob>& vertexShaderBlob)
{
    // hash the input layout
    auto hash = hash_input_layout(layout_desc);
    if (layout_map.contains(hash))
        return layout_map[hash].Get();

    ComPtr<ID3D11InputLayout> layout;
    if (FAILED(device->CreateInputLayout(
        layout_desc.data(),
        static_cast<UINT>(layout_desc.size()),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &layout)))
    {
        throw std::runtime_error("Failed to create Input Layout");
    }

    layout_map[hash] = layout;
    return layout.Get();
}

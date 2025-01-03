#include "d3d11_layout_assembler.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <iostream>

template <typename T>
void hash_combine(std::size_t& seed, const T& v) {
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

void D3D11LayoutAssembler::add_input(const D3D11_INPUT_ELEMENT_DESC& input)
{
    layout_desc_.push_back(input);
}

ID3D11InputLayout* D3D11LayoutAssembler::find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout)
{
	auto hash = hash_input_layout(layout);
	if (layout_map.contains(hash)) return layout_map[hash].layout.Get();

	return nullptr;
}

Layout* D3D11LayoutAssembler::find_layout(ID3D11InputLayout* layout)
{
	std::lock_guard lock(layout_mutex_);
	if (layout_logical_map.contains(layout)) return layout_logical_map[layout];
	return nullptr;
}

#define find_layout(layout_d) auto hash = hash_input_layout(layout_d); \
	if (layout_map.contains(hash)) return layout_map[hash].layout.Get(); \


std::optional<ComPtr<ID3D11InputLayout>> D3D11LayoutAssembler::create_input_layout_manual(
	ID3D11Device* const device,
	ID3DBlob* const vertex_shader_blob)
{
	std::lock_guard lock(layout_mutex_);
    // hash the input layout
	find_layout(layout_desc_)

    ComPtr<ID3D11InputLayout> layout;
    if (FAILED(device->CreateInputLayout(
        layout_desc_.data(),
        static_cast<UINT>(layout_desc_.size()),
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(),
        &layout)))
    {
		std::cerr << "D3D11: Failed to create Input Layout manual" << std::endl;
        return {};
    }

    layout_map[hash] = { layout, layout_desc_ };
	layout_logical_map[layout.Get()] = &layout_map[hash];

    return layout;
}

ID3D11InputLayout* D3D11LayoutAssembler::create_input_layout_reflection(
	ID3D11Device* const device,
	ID3DBlob* const vertex_shader_blob, bool interleaved)
{
    ComPtr<ID3D11ShaderReflection> pVertexShaderReflection;
    if (FAILED(D3DReflect(vertex_shader_blob->GetBufferPointer(), 
        vertex_shader_blob->GetBufferSize(), 
        IID_ID3D11ShaderReflection, 
        &pVertexShaderReflection)))
    {
		std::cerr << "D3D11: Input layout reflection failed" << std::endl;
		return nullptr;
    }

    D3D11_SHADER_DESC shaderDesc;
    pVertexShaderReflection->GetDesc(&shaderDesc);

    UINT slot = 0;

    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
    for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		// Ignore system attributes
		if (paramDesc.SystemValueType == D3D_NAME_VERTEX_ID 
            || paramDesc.SystemValueType == D3D_NAME_PRIMITIVE_ID
            || paramDesc.SystemValueType == D3D_NAME_INSTANCE_ID) continue;

        D3D11_INPUT_ELEMENT_DESC elementDesc;
        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = slot;
        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        //// TODO: INSTANCING
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        // Determine DXGI format
        if (paramDesc.Mask == 1)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (paramDesc.Mask <= 3)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (paramDesc.Mask <= 7)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (paramDesc.Mask <= 15)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else
        {
			std::cerr << "D3D11: check mask " << paramDesc.Mask << std::endl;
        }

        if (!interleaved) slot++;

        //save element desc
        inputLayoutDesc.push_back(elementDesc);
    }

    if (inputLayoutDesc.empty())
    {
		return {};
    }

	// hash and check if layout already exists
	std::lock_guard lock(layout_mutex_);
	find_layout(inputLayoutDesc)

    ComPtr<ID3D11InputLayout> layout;
    if (FAILED(device->CreateInputLayout(
        inputLayoutDesc.data(),
        inputLayoutDesc.size(),
        vertex_shader_blob->GetBufferPointer(), 
        vertex_shader_blob->GetBufferSize(), 
        &layout)))
    {
		std::cerr << "D3D11: Failed to create Input Layout reflection" << std::endl;
		return nullptr;
    }

    layout_map[hash] = { layout, std::move(inputLayoutDesc) };
	layout_logical_map[layout.Get()] = &layout_map[hash];

	return layout.Get();
}

void D3D11LayoutAssembler::dispose()
{
	std::lock_guard lock(layout_mutex_);
	layout_map.clear();
	layout_logical_map.clear();
	layout_desc_.clear();
}
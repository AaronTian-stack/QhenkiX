#include "d3d11_layout_assembler.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <string>

using namespace qhenki::gfx;

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
    m_layout_desc_.push_back(input);
}

ID3D11InputLayout* D3D11LayoutAssembler::find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout)
{
	auto hash = hash_input_layout(layout);
	if (m_layout_map_.contains(hash)) return m_layout_map_[hash].layout.Get();

	return nullptr;
}

D3D11Layout* D3D11LayoutAssembler::find_layout(ID3D11InputLayout* layout)
{
	std::lock_guard lock(m_layout_mutex_);
	if (m_layout_logical_map_.contains(layout)) return m_layout_logical_map_[layout];
	return nullptr;
}

#define find_layout(layout_d) auto hash = hash_input_layout(layout_d); \
	if (m_layout_map_.contains(hash)) return m_layout_map_[hash].layout.Get(); \


std::optional<ComPtr<ID3D11InputLayout>> D3D11LayoutAssembler::create_input_layout_manual(
	ID3D11Device* const device,
	ID3DBlob* const vertex_shader_blob)
{
	std::lock_guard lock(m_layout_mutex_);
    // hash the input layout
	find_layout(m_layout_desc_)

    ComPtr<ID3D11InputLayout> layout;
    if (FAILED(device->CreateInputLayout(
        m_layout_desc_.data(),
        static_cast<UINT>(m_layout_desc_.size()),
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(),
        &layout)))
    {
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create Input Layout manual\n");
        return {};
    }

    m_layout_map_[hash] = { layout, m_layout_desc_ };
	m_layout_logical_map_[layout.Get()] = &m_layout_map_[hash];

    return layout;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> D3D11LayoutAssembler::create_input_layout_desc(ID3D11ShaderReflection* vs_reflection, const bool interleaved)
{
	assert(vs_reflection);

    D3D11_SHADER_DESC shader_desc;
    vs_reflection->GetDesc(&shader_desc);

    UINT slot = 0;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
    input_layout_desc.reserve(shader_desc.InputParameters);
    for (uint32_t i = 0; i < shader_desc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        vs_reflection->GetInputParameterDesc(i, &paramDesc);

        // Ignore system attributes
        if (paramDesc.SystemValueType == D3D_NAME_VERTEX_ID
            || paramDesc.SystemValueType == D3D_NAME_PRIMITIVE_ID
            || paramDesc.SystemValueType == D3D_NAME_INSTANCE_ID) continue;

        D3D11_INPUT_ELEMENT_DESC elementDesc =
        {
            .SemanticName = paramDesc.SemanticName,
            .SemanticIndex = paramDesc.SemanticIndex,
            .InputSlot = slot,
            .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            //// TODO: INSTANCING
            .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

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
            std::wstring mask_str;
			mask_str.reserve(4);
			for (int i = 0; i < 4; i++)
			{
				mask_str.push_back((paramDesc.Mask & (1 << i)) ? L'1' : L'0');
			}
			OutputDebugString((L"D3D11: Invalid mask " + mask_str + L"\n").c_str());
        }

        if (!interleaved) slot++;

        // save element desc
        input_layout_desc.push_back(elementDesc);
    }

	return input_layout_desc;
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
		OutputDebugString(L"Qhenki D3D11 ERROR: Input layout reflection failed\n");
		return nullptr;
    }

    D3D11_SHADER_DESC shader_desc;
    pVertexShaderReflection->GetDesc(&shader_desc);

	std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc = create_input_layout_desc(pVertexShaderReflection.Get(), interleaved);

    if (input_layout_desc.empty())
    {
		return {};
    }

	// hash and check if layout already exists
	std::lock_guard lock(m_layout_mutex_);
	find_layout(input_layout_desc)

    ComPtr<ID3D11InputLayout> layout;
    if (FAILED(device->CreateInputLayout(
        input_layout_desc.data(),
        input_layout_desc.size(),
        vertex_shader_blob->GetBufferPointer(), 
        vertex_shader_blob->GetBufferSize(), 
        &layout)))
    {
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create Input Layout reflection\n");
		return nullptr;
    }

    m_layout_map_[hash] = { layout, std::move(input_layout_desc) };
	m_layout_logical_map_[layout.Get()] = &m_layout_map_[hash];

	return layout.Get();
}

void D3D11LayoutAssembler::dispose()
{
	std::lock_guard lock(m_layout_mutex_);
	m_layout_map_.clear();
	m_layout_logical_map_.clear();
	m_layout_desc_.clear();
}

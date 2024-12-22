#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <mutex>
#include <wrl/client.h>
#include <tsl/robin_map.h>

using Microsoft::WRL::ComPtr;

class D3D11LayoutAssembler
{
	std::mutex layout_mutex_; // compile shaders from multiple threads?
	tsl::robin_map<size_t, ComPtr<ID3D11InputLayout>> layout_map;
	std::vector<D3D11_INPUT_ELEMENT_DESC> layout_desc_;

public:
	void add_input(const D3D11_INPUT_ELEMENT_DESC& input);
	ID3D11InputLayout* find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout);
	// Creates input layout based off current state of layout_desc
	ID3D11InputLayout* create_input_layout_manual(const ComPtr<ID3D11Device>& device, 
		const ComPtr<ID3DBlob>& vertex_shader_blob);
	
    /**
     * @brief Creates an input layout using shader reflection.
     * 
     * Creates input layout using shader reflection
     * 
     * @param device D3D11 device
     * @param vertex_shader_blob Compiled vertex shader blob
	 * @param interleaved If true, slots will be incremented by 1 for each vertex attribute
     * @return Pointer to the created ID3D11InputLayout
     */
    ID3D11InputLayout* create_input_layout_reflection(const ComPtr<ID3D11Device>& device,
        const ComPtr<ID3DBlob>& vertex_shader_blob, bool interleaved);
};

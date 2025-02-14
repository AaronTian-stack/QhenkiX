#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct D3D12ReflectionData
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc;
	std::vector<std::string> input_element_semantic_names;
	void clear()
	{
		input_layout_desc.clear();
		input_element_semantic_names.clear();
	}
};

struct D3D12Pipeline
{
	D3D12ReflectionData reflection_data; // Clear this after creation!
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc = nullptr; // Temp for deferred compilation
	ComPtr<ID3D12PipelineState> pipeline_state = nullptr;
	bool deferred = false;
};
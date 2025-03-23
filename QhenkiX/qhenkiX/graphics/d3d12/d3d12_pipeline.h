﻿#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	struct D3D12Pipeline
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc; // Clear this after creation!
		D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc = nullptr; // Temp for deferred compilation
		D3D12_PRIMITIVE_TOPOLOGY primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; // Needed for command list
		ComPtr<ID3D12PipelineState> pipeline_state = nullptr;
		bool deferred = false;
	};
}
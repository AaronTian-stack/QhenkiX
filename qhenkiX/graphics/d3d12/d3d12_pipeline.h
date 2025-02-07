#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct D3D12Pipeline
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc = nullptr; // Temp for deferred compilation
	ComPtr<ID3D12PipelineState> pipeline_state = nullptr;
	bool deferred = false;
};
#pragma once
#include <cstdint>
#include <d3d12.h>

#include "shader.h"

namespace vendetta
{
	struct RasterizerDesc
	{
		D3D12_FILL_MODE                       fill_mode = D3D12_FILL_MODE_SOLID; // 4
		D3D12_CULL_MODE                       cull_mode = D3D12_CULL_MODE_NONE; // 4
		BOOL                                  front_counter_clockwise = TRUE; // 4
		int                                   depth_bias = 0; // 4
		float                                 depth_bias_clamp = 0.f; // 4
		float                                 slope_scaled_depth_bias = 0.f; // 4
		BOOL                                  depth_clip_enable = 0; // 4
	};

	struct DepthStencilDesc
	{
		D3D12_DEPTH_STENCILOP_DESC            front_face; // 16
		D3D12_DEPTH_STENCILOP_DESC            back_face; // 16
		uint32_t                              depth_enable = TRUE; // 4
		D3D12_DEPTH_WRITE_MASK                depth_write_mask = D3D12_DEPTH_WRITE_MASK_ZERO; // 4
		D3D12_COMPARISON_FUNC                 depth_func = D3D12_COMPARISON_FUNC_LESS; // 4
		BOOL                                  stencil_enable = FALSE; // 4
		uint8_t                               stencil_read_mask; // 1
		uint8_t                               stencil_write_mask; // 1
		// vulkan uses a single struct for both front and back face (read/write mask)
	};

	struct InputLayoutDesc
	{
		uint32_t							num_elements;
		D3D12_INPUT_ELEMENT_DESC*			elements;
	};

	struct PipelineDesc
	{
		D3D12_BLEND_DESC* blend_desc = nullptr; // 328
		DepthStencilDesc* depth_stencil_state = nullptr; // 52
		RasterizerDesc* rasterizer_state = nullptr; // 28
		InputLayoutDesc* input_layout = nullptr; // 16
		DXGI_SAMPLE_DESC* multisample_desc = nullptr; // 8
		D3D12_PRIMITIVE_TOPOLOGY_TYPE* primitive_topology_type = nullptr; // 4
		int num_render_targets = -1;
		DXGI_FORMAT RTVFormats[8]{};
	};
	class Pipeline
	{
		PipelineDesc desc{}; // for DX12 should be freed after pipeline creation
		Shader* shader = nullptr;
		sPtr<void> internal_state;
	};
}

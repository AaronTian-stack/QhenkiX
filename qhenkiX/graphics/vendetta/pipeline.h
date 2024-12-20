#pragma once
#include <cstdint>
#include <d3d12.h>
#include "application.h"

namespace vendetta
{
	struct RasterizerDesc
	{
		D3D12_FILL_MODE                       fill_mode = D3D12_FILL_MODE_SOLID;
		D3D12_CULL_MODE                       cull_mode = D3D12_CULL_MODE_NONE;
		BOOL                                  front_counter_clockwise = TRUE;
		int                                   depth_bias = 0;
		float                                 depth_bias_clamp = 0.f;
		float                                 slope_scaled_depth_bias = 0.f;
		BOOL                                  depth_clip_enable = 0;
	};

	struct DepthStencilDesc
	{
		uint32_t                              depth_enable = TRUE;
		D3D12_DEPTH_WRITE_MASK                depth_write_mask = D3D12_DEPTH_WRITE_MASK_ZERO;
		D3D12_COMPARISON_FUNC                 depth_func = D3D12_COMPARISON_FUNC_LESS;
		BOOL                                  stencil_enable = FALSE;
		uint8_t                               stencil_read_mask;
		uint8_t                               stencil_write_mask;
		D3D12_DEPTH_STENCILOP_DESC            front_face;
		D3D12_DEPTH_STENCILOP_DESC            back_face;
		// vulkan uses a single struct for both front and back face (read/write mask)
	};

	struct InputLayoutDesc
	{
		uint32_t							num_elements;
		D3D12_INPUT_ELEMENT_DESC*			elements;
	};

	struct PipelineDesc
	{
		RasterizerDesc rasterizer_state;
		DepthStencilDesc depth_stencil_state;
		InputLayoutDesc input_layout;
		DXGI_SAMPLE_DESC multisample_desc;
		int num_render_targets;
		D3D12_BLEND_DESC blend_desc;
		DXGI_FORMAT RTVFormats[8];
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type;
	};
	class Pipeline
	{
		PipelineDesc* desc;
		Shader* shader = nullptr;
		sPtr<void> internal_state;
	};
}

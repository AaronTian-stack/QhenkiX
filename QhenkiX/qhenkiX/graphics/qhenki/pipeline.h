#pragma once
#include <cstdint>
#include <d3d12.h>
#include <optional>

#include "enums.h"

namespace qhenki::gfx
{
	struct RasterizerDesc
	{
		D3D12_FILL_MODE                       fill_mode = D3D12_FILL_MODE_SOLID; // 4
		D3D12_CULL_MODE                       cull_mode = D3D12_CULL_MODE_NONE; // 4
		BOOL                                  front_counter_clockwise = TRUE; // 4
		int                                   depth_bias = 0; // 4
		float                                 depth_bias_clamp = 0.f; // 4
		float                                 slope_scaled_depth_bias = 0.f; // 4
		BOOL                                  depth_clip_enable = TRUE; // 4
		// Always uses alpha MSAA
		// No AA lines
		// No forced Sample Count
		// TODO: Conservative Rasterization?
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
		// Vulkan uses a single struct for both front and back face (read/write mask)
	};

	struct InputLayoutDesc
	{
		uint32_t							num_elements;
		D3D12_INPUT_ELEMENT_DESC*			elements;
	};

	struct GraphicsPipelineDesc
	{
		std::optional<D3D12_BLEND_DESC> blend_desc;
		std::optional<DepthStencilDesc> depth_stencil_state;
		std::optional<RasterizerDesc> rasterizer_state;
		std::optional<InputLayoutDesc> input_layout;
		std::optional<DXGI_SAMPLE_DESC> multisample_desc;
		PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
		int num_render_targets = -1; // If this is <= 0, pipeline is lazily created based off what render target is bound at draw time
		std::array<DXGI_FORMAT, 8> rtv_formats{};
		BOOL interleaved = FALSE; // Whether vertex data is interleaved or not, used during reflection
	};

	struct GraphicsPipeline
	{
		sPtr<void> internal_state;
	};

	struct LayoutBinding
	{
		uint32_t binding;
		uint32_t count;
		D3D12_DESCRIPTOR_RANGE_TYPE type;
		// TODO: stage flags
	};

	struct PushRange
	{
		uint32_t offset;
		uint32_t size;
		uint32_t binding; // Not relevant in Vulkan
		// TODO: stage flags
	};

#define INFINITE_DESCRIPTORS 0xFFFFFFFF

	struct PipelineLayoutDesc
	{
		std::vector<PushRange> push_ranges; // TODO: use small vector
		std::array<std::vector<LayoutBinding>, 4> spaces; // TODO: use small vector
	};

	struct PipelineLayout
	{
		sPtr<void> internal_state;
	};
}

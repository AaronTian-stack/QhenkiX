#pragma once

#define NOMINMAX
#include "d3d11_layout_assembler.h"

struct D3D11GraphicsPipeline
{
	void* vertex_shader_ = nullptr; // D3D11Shader*
	void* pixel_shader_ = nullptr; // D3D11Shader*
	ID3D11InputLayout* input_layout_ = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ComPtr<ID3D11RasterizerState> rasterizer_state_ = nullptr;
	ComPtr<ID3D11BlendState> blend_state_ = nullptr;
	ComPtr<ID3D11DepthStencilState> depth_stencil_state_ = nullptr;
	// Binds both vertex pixel shaders and pipeline states. If any state struct is null, the state is not changed.
	void bind(ID3D11DeviceContext* const context);
};

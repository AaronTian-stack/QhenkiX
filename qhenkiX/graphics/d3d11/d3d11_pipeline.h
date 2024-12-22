#pragma once

#define NOMINMAX
#include "d3d11_shader.h"

struct D3D11GraphicsPipeline
{
	void* vertex_shader_ = nullptr; // D3D11Shader*
	void* pixel_shader_ = nullptr; // D3D11Shader*
	ComPtr<ID3D11InputLayout> input_layout_ = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ComPtr<ID3D11RasterizerState> rasterizer_state_ = nullptr;
	ComPtr<ID3D11BlendState> blend_state_ = nullptr;
	ComPtr<ID3D11DepthStencilState> depth_stencil_state_ = nullptr;
	// if state struct is null, state is not changed
	void bind(const ComPtr<ID3D11DeviceContext>& context);
};

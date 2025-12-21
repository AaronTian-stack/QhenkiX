#pragma once

#include "d3d11_layout_assembler.h"

namespace qhenki::gfx
{
	struct D3D11GraphicsPipeline
	{
		void* vertex_shader = nullptr; // D3D11Shader*
		void* pixel_shader = nullptr; // D3D11Shader*
		ID3D11InputLayout* input_layout = nullptr;
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ComPtr<ID3D11RasterizerState> rasterizer_state = nullptr;
		ComPtr<ID3D11BlendState> blend_state = nullptr;
		ComPtr<ID3D11DepthStencilState> depth_stencil_state = nullptr;
		// Binds both vertex pixel shaders and pipeline states. If any state struct is null, the state is not changed.
		void bind(ID3D11DeviceContext* const context);
	};
}

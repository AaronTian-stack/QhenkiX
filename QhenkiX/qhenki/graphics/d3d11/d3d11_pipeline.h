#pragma once

#include "d3d11_layout_assembler.h"

namespace qhenki::gfx
{
struct D3D11GraphicsPipeline
{
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ID3D11InputLayout* input_layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    ComPtr<ID3D11BlendState> blend_state;
    ComPtr<ID3D11DepthStencilState> depth_stencil_state;
    // Binds both vertex pixel shaders and pipeline states. If any state struct is null, the state is not changed.
    void bind(ID3D11DeviceContext* context) const;
};
} // namespace qhenki::gfx

#include "d3d11_pipeline.h"

#include <cassert>
#include "d3d11_shader.h"

using namespace qhenki::gfx;

void D3D11GraphicsPipeline::bind(ID3D11DeviceContext* const context) const
{
    context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    context->PSSetShader(pixel_shader.Get(), nullptr, 0);

    if (input_layout)
    {
        context->IASetInputLayout(input_layout);
    }
    else
    {
        context->IASetInputLayout(nullptr);
    }
    if (topology)
    {
        context->IASetPrimitiveTopology(topology);
    }
    if (rasterizer_state)
    {
        context->RSSetState(rasterizer_state.Get());
    }
    else
    {
        context->RSSetState(nullptr);
    }
    if (blend_state)
    {
        context->OMSetBlendState(blend_state.Get(), nullptr, 0xffffffff);
    }
    else
    {
        context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }
    if (depth_stencil_state)
    {
        context->OMSetDepthStencilState(depth_stencil_state.Get(), 0);
    }
    else
    {
        context->OMSetDepthStencilState(nullptr, 0);
    }
}

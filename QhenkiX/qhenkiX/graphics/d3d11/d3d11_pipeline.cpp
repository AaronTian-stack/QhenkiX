#include "d3d11_pipeline.h"

#include <cassert>
#include "d3d11_shader.h"

using namespace qhenki::gfx;

void D3D11GraphicsPipeline::bind(ID3D11DeviceContext* const context)
{
	if (const auto shader = static_cast<D3D11Shader*>(vertex_shader))
	{
		const auto vs = std::get_if<D3D11VertexShader>(&shader->m_shader);  
		assert(vs);
		context->VSSetShader(vs->vertex_shader.Get(), nullptr, 0);
	}
	if (const auto shader = static_cast<D3D11Shader*>(pixel_shader))
	{
		const auto ps = std::get_if<ComPtr<ID3D11PixelShader>>(&shader->m_shader);
		assert(ps);
		context->PSSetShader(ps->Get(), nullptr, 0);
	}
	if (input_layout)
	{
		context->IASetInputLayout(input_layout);
	}
	if (topology)
	{
		context->IASetPrimitiveTopology(topology);
	}
	if (rasterizer_state)
	{
		context->RSSetState(rasterizer_state.Get());
	}
	if (blend_state)
	{
		context->OMSetBlendState(blend_state.Get(), nullptr, 0xffffffff);
	}
	if (depth_stencil_state)
	{
		context->OMSetDepthStencilState(depth_stencil_state.Get(), 0);
	}
}

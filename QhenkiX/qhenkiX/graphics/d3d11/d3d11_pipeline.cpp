#include "d3d11_pipeline.h"

#include <cassert>
#include "d3d11_shader.h"

using namespace qhenki::gfx;

void D3D11GraphicsPipeline::bind(ID3D11DeviceContext* const context)
{
	if (const auto shader = static_cast<D3D11Shader*>(vertex_shader_))
	{
		const auto vs = std::get_if<D3D11VertexShader>(&shader->m_shader_);  
		assert(vs);
		context->VSSetShader(vs->vertex_shader.Get(), nullptr, 0);
	}
	if (const auto shader = static_cast<D3D11Shader*>(pixel_shader_))
	{
		const auto ps = std::get_if<ComPtr<ID3D11PixelShader>>(&shader->m_shader_);
		assert(ps);
		context->PSSetShader(ps->Get(), nullptr, 0);
	}
	if (input_layout_) context->IASetInputLayout(input_layout_);
	if (topology_) context->IASetPrimitiveTopology(topology_);
	if (rasterizer_state_) context->RSSetState(rasterizer_state_.Get());
	if (blend_state_) context->OMSetBlendState(blend_state_.Get(), nullptr, 0xffffffff);
	if (depth_stencil_state_) context->OMSetDepthStencilState(depth_stencil_state_.Get(), 0);
}

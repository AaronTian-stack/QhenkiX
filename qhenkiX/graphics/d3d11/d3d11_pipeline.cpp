#include "d3d11_pipeline.h"

#include <cassert>

void D3D11GraphicsPipeline::bind(const ComPtr<ID3D11DeviceContext>& context)
{
#ifdef _DEBUG
	{
		int vsc = 0;
		int psc = 0;
		int cc = 0;
		for (auto i = 0; i < 3; i++)
		{
			if (vertex_shader_ && static_cast<D3D11Shader*>(vertex_shader_)->vertex) vsc++;
			if (pixel_shader_ && static_cast<D3D11Shader*>(pixel_shader_)->pixel) psc++;
			// TODO: COMPUTE if (compute_shader_ && static_cast<D3D11Shader*>(compute_shader_)->compute) cc++;
		}
		assert(vsc <= 1 && "Vertex shader count (vsc) should be <= 1");
		assert(psc <= 1 && "Pixel shader count (psc) should be <= 1");
	}
#endif
	if (auto shader = static_cast<D3D11Shader*>(vertex_shader_))
	{
		assert(shader->vertex);
		context->VSSetShader(shader->vertex.Get(), nullptr, 0);
	}
	if (auto shader = static_cast<D3D11Shader*>(pixel_shader_))
	{
		assert(shader->pixel);
		context->PSSetShader(shader->pixel.Get(), nullptr, 0);
	}
	if (input_layout_) context->IASetInputLayout(input_layout_.Get());
	if (topology_) context->IASetPrimitiveTopology(topology_);
	if (rasterizer_state_) context->RSSetState(rasterizer_state_.Get());
	if (blend_state_) context->OMSetBlendState(blend_state_.Get(), nullptr, 0xffffffff);
	if (depth_stencil_state_) context->OMSetDepthStencilState(depth_stencil_state_.Get(), 0);
}

#include "exampleapp.h"

void ExampleApp::create()
{
	ComPtr<ID3DBlob> vertex_shader_blob;
	vertex_shader = d3d11.create_vertex_shader(L"base-shaders/BaseShader.vs.hlsl", vertex_shader_blob, nullptr);
	pixel_shader = d3d11.create_pixel_shader(L"base-shaders/BaseShader.ps.hlsl", nullptr);
}

void ExampleApp::render()
{
	d3d11.debug_clear();

	// unbind all the buffers and input layout
	d3d11.device_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	d3d11.device_context->IASetVertexBuffers(1, 0, nullptr, nullptr, nullptr);
	d3d11.device_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	d3d11.device_context->IASetInputLayout(nullptr);

	d3d11.device_context->VSSetShader(
		vertex_shader.Get(),
		nullptr,
		0);
	d3d11.device_context->PSSetShader(
		pixel_shader.Get(),
		nullptr,
		0);
	d3d11.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	d3d11.device_context->Draw(3, 0);

	d3d11.present(1);
}

void ExampleApp::resize(int width, int height)
{
	
}

void ExampleApp::destroy()
{
	
}

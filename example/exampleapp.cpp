#include "exampleapp.h"

void ExampleApp::create()
{
	ComPtr<ID3DBlob> vertex_shader_blob;
	vertex_shader = d3d11_.create_vertex_shader(L"base-shaders/BaseShader.vs.hlsl", vertex_shader_blob);
	pixel_shader = d3d11_.create_pixel_shader(L"base-shaders/BaseShader.ps.hlsl");
}

void ExampleApp::render()
{
	d3d11_.clear_set_default();
	auto s = window_.get_display_size();
	D3D11_VIEWPORT viewport = 
	{
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<float>(s.x),
		.Height = static_cast<float>(s.y),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	d3d11_.device_context->RSSetViewports(
		1,
		&viewport);

	d3d11_.device_context->VSSetShader(
		vertex_shader.Get(),
		nullptr,
		0);
	d3d11_.device_context->PSSetShader(
		pixel_shader.Get(),
		nullptr,
		0);
	d3d11_.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	d3d11_.device_context->Draw(3, 0);

	d3d11_.present(1);
}

void ExampleApp::resize(int width, int height)
{
	
}

void ExampleApp::destroy()
{
	vertex_shader.Reset();
	pixel_shader.Reset();
}

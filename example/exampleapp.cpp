#include "exampleapp.h"

void ExampleApp::create()
{
	// create shaders
	context_->create_shader(vertex_shader, L"base-shaders/BaseShader.vs.hlsl", vendetta::ShaderType::VERTEX, {});
	context_->create_shader(pixel_shader, L"base-shaders/BaseShader.ps.hlsl", vendetta::ShaderType::PIXEL, {});
	// create pipeline
	context_->create_pipeline(pipeline, vertex_shader, pixel_shader);
}

void ExampleApp::render()
{
	// TODO: command list
	// Clear back buffer / Start render pass
	context_->start_render_pass(cmd_list, swapchain_, nullptr);

	// Set viewport
	auto s = window_.get_display_size();
	D3D12_VIEWPORT viewport =
	{
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<float>(s.x),
		.Height = static_cast<float>(s.y),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	context_->set_viewports(1, &viewport);
	// Bind pipeline
	context_->bind_pipeline(cmd_list, pipeline);
	// TODO: bind buffer
	// Draw
	context_->draw(cmd_list, 3, 0);
	// Present
	context_->present(swapchain_);
}

void ExampleApp::resize(int width, int height)
{
	
}

void ExampleApp::destroy()
{
	// destroy pipeline then shaders
}

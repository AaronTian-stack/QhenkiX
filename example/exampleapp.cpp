#include "exampleapp.h"

void ExampleApp::create()
{
	// Create shaders
	context_->create_shader(vertex_shader, L"base-shaders/BaseShader.vs.hlsl", qhenki::ShaderType::VERTEX_SHADER, {});
	context_->create_shader(pixel_shader, L"base-shaders/BaseShader.ps.hlsl", qhenki::ShaderType::PIXEL_SHADER, {});

	// Create pipeline
	qhenki::GraphicsPipelineDesc pipeline_desc = {};
	context_->create_pipeline(pipeline_desc, pipeline, vertex_shader, pixel_shader);

	// TODO: create queue
	// TODO: allocate command pool from queue
	// TODO: allocate command list from command pool

	// Create vertex buffer
	const auto vertices = std::array{
		0.0f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
	};
	qhenki::BufferDesc desc =
	{
		.size = vertices.size() * sizeof(float),
		.usage = qhenki::BufferUsage::VERTEX,
		.visibility = qhenki::BufferVisibility::GPU_ONLY
	};
	context_->create_buffer(desc, vertices.data(), vertex_buffer);
}

void ExampleApp::render()
{
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
	const unsigned int offset = 0;
	context_->bind_vertex_buffers(cmd_list, 0, 1, &vertex_buffer, &offset);
	// Draw
	context_->draw(cmd_list, 3, 0);
	// TODO: submit command list

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

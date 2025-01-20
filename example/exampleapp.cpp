#include "exampleapp.h"

void ExampleApp::create()
{
	// Create shaders
	context_->create_shader(vertex_shader, L"base-shaders/BaseShader.vs.hlsl", qhenki::ShaderType::VERTEX_SHADER, {});
	context_->create_shader(pixel_shader, L"base-shaders/BaseShader.ps.hlsl", qhenki::ShaderType::PIXEL_SHADER, {});

	// Create pipeline
	qhenki::GraphicsPipelineDesc pipeline_desc =
	{
		.interleaved = TRUE,
	};
	context_->create_pipeline(pipeline_desc, pipeline, vertex_shader, pixel_shader);

	// TODO: create queue
	// TODO: allocate command pool from queue
	// TODO: allocate command list from command pool

	// Create vertex buffer
	const auto vertices = std::array{
		0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
	};
	qhenki::BufferDesc desc =
	{
		.size = vertices.size() * sizeof(float),
		.usage = qhenki::BufferUsage::VERTEX,
		.visibility = qhenki::BufferVisibility::GPU_ONLY
	};
	context_->create_buffer(desc, vertices.data(), vertex_buffer, L"Interleaved Position/Color Buffer");

	const auto indices = std::array{ 0u, 1u, 2u };
	qhenki::BufferDesc index_desc =
	{
		.size = indices.size() * sizeof(uint32_t),
		.usage = qhenki::BufferUsage::INDEX,
		.visibility = qhenki::BufferVisibility::GPU_ONLY
	};
	context_->create_buffer(index_desc, indices.data(), index_buffer, L"Index Buffer");

	qhenki::BufferDesc matrix_desc =
	{
		.size = sizeof(CameraMatrices),
		.usage = qhenki::BufferUsage::UNIFORM,
		.visibility = qhenki::BufferVisibility::CPU_SEQUENTIAL
	};
	context_->create_buffer(matrix_desc, nullptr, matrix_buffer, L"Matrix Buffer");
}

void ExampleApp::render()
{
	const auto seconds_elapsed = static_cast<float>(SDL_GetTicks64()) / 1000.f;

	// Update matrices
	const XMVECTORF32 eye_pos = { sin(seconds_elapsed) * 2.0f, 0.0f, cos(seconds_elapsed) * 2.0f };
	const XMVECTORF32 focus_pos = { 0.0f, 0.0f, 0.0f };
	const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f };
    const auto view = XMMatrixLookAtLH(eye_pos, focus_pos, up);
	const auto dim = this->window_.get_display_size();
	const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(dim.x) / static_cast<float>(dim.y), 
		0.01f, 100.0f);
	const auto prod = XMMatrixTranspose(XMMatrixMultiply(view, proj));
	XMStoreFloat4x4(&matrices.viewProj, prod);
	XMStoreFloat4x4(&matrices.invViewProj, XMMatrixInverse(nullptr, prod));

	// Update matrix buffer
	const auto buffer_pointer = context_->map_buffer(matrix_buffer);
	memcpy(buffer_pointer, &matrices, sizeof(CameraMatrices));
	context_->unmap_buffer(matrix_buffer);

	// Clear back buffer / Start render pass
	context_->start_render_pass(cmd_list, swapchain_, nullptr);

	// Set viewport
	const D3D12_VIEWPORT viewport =
	{
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<float>(dim.x),
		.Height = static_cast<float>(dim.y),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	context_->set_viewports(1, &viewport);
	// Bind pipeline
	context_->bind_pipeline(cmd_list, pipeline);

	/**
	* TODO: Bind table to pipeline
	* should follow builder pattern. add table/constant bindings, then backend
	* does lookup for corresponding root signature (auto generated via reflection)
	*/

	const unsigned int offset = 0;
	context_->bind_vertex_buffers(cmd_list, 0, 1, &vertex_buffer, &offset);
	context_->bind_index_buffer(cmd_list, index_buffer, DXGI_FORMAT_R32_UINT, 0);

	// Draw
	context_->draw_indexed(cmd_list, 3, 0, 0);
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

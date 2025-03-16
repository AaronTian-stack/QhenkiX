#include "exampleapp.h"

void ExampleApp::create()
{
	auto shader_model = m_context_->is_compatability() ? 
		qhenki::gfx::ShaderModel::SM_5_0 : qhenki::gfx::ShaderModel::SM_6_0;

	auto compiler_flags = CompilerInput::NONE;

	std::vector<std::wstring> defines;
	defines.reserve(1);
	if (m_context_->is_compatability())
	{
		defines.push_back(L"DX11");
	}
	else
	{
		defines.push_back(L"DX12");
	}

	// Create shaders
	CompilerInput vertex_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::VERTEX_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"vs_main",
		.min_shader_model = shader_model,
		.defines = defines,
	};
	m_context_->create_shader_dynamic(nullptr, m_vertex_shader_, vertex_shader);

	CompilerInput pixel_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::PIXEL_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"ps_main",
		.min_shader_model = shader_model,
		.defines = defines,
	};
	m_context_->create_shader_dynamic(nullptr, m_pixel_shader_, pixel_shader);

	// Create pipeline
	qhenki::gfx::GraphicsPipelineDesc pipeline_desc =
	{
		.num_render_targets = 1,
		.rtv_formats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		.interleaved = TRUE,
	};
	m_context_->create_pipeline(pipeline_desc, m_pipeline_, m_vertex_shader_, m_pixel_shader_, nullptr, nullptr, L"triangle_pipeline");

	// Create queue(s)
	m_context_->create_queue(qhenki::gfx::QueueType::GRAPHICS, m_graphics_queue_);
	// Allocate command pool(s)/allocator(s) from queue
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		m_context_->create_command_pool(m_cmd_pools_[i], m_graphics_queue_);
	}

	// Create vertex buffer
	const auto vertices = std::array{
		0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
	};
	qhenki::gfx::BufferDesc desc =
	{
		.size = vertices.size() * sizeof(float),
		.usage = qhenki::gfx::BufferUsage::VERTEX,
		.visibility = qhenki::gfx::BufferVisibility::GPU
	};
	m_context_->create_buffer(desc, vertices.data(), m_vertex_buffer_, L"Interleaved Position/Color Buffer");

	const auto indices = std::array{ 0u, 1u, 2u };
	qhenki::gfx::BufferDesc index_desc =
	{
		.size = indices.size() * sizeof(uint32_t),
		.usage = qhenki::gfx::BufferUsage::INDEX,
		.visibility = qhenki::gfx::BufferVisibility::GPU
	};
	m_context_->create_buffer(index_desc, indices.data(), m_index_buffer_, L"Index Buffer");

	// Make 2 matrix constant buffers for double buffering
	// TODO: persistent mapping flag
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		qhenki::gfx::BufferDesc matrix_desc =
		{
			.size = sizeof(CameraMatrices),
			.usage = qhenki::gfx::BufferUsage::UNIFORM,
			.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
		};
		m_context_->create_buffer(matrix_desc, nullptr, m_matrix_buffers_[i], L"Matrix Buffer");
	}
}

void ExampleApp::render()
{
	const auto seconds_elapsed = static_cast<float>(SDL_GetTicks()) / 1000.f;

	// Update matrices
	const XMVECTORF32 eye_pos = { sinf(seconds_elapsed) * 2.0f, 0.0f, cosf(seconds_elapsed) * 2.0f };
	const XMVECTORF32 focus_pos = { 0.0f, 0.0f, 0.0f };
	const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f };
    const auto view = XMMatrixLookAtLH(eye_pos, focus_pos, up);
	const auto dim = this->m_window_.get_display_size();
	const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(dim.x) / static_cast<float>(dim.y), 
		0.01f, 100.0f);
	const auto prod = XMMatrixTranspose(XMMatrixMultiply(view, proj));
	XMStoreFloat4x4(&matrices_.viewProj, prod);
	XMStoreFloat4x4(&matrices_.invViewProj, XMMatrixInverse(nullptr, prod));

	// Update matrix buffer
	const auto buffer_pointer = m_context_->map_buffer(m_matrix_buffers_[get_frame_index()]);
	throw_if_failed(buffer_pointer);
	memcpy(buffer_pointer, &matrices_, sizeof(CameraMatrices));
	m_context_->unmap_buffer(m_matrix_buffers_[get_frame_index()]);

	m_context_->reset_command_pool(m_cmd_pools_[get_frame_index()]);

	qhenki::gfx::CommandList cmd_list;
	// Create a command list in the open state
	m_context_->create_command_list(cmd_list, m_cmd_pools_[get_frame_index()]);

	// TODO: resource transition
	// m_context_->

	// Clear back buffer / Start render pass
	m_context_->start_render_pass(cmd_list, m_swapchain_, nullptr, get_frame_index());

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
	const D3D12_RECT scissor_rect =
	{
		.left = 0,
		.top = 0,
		.right = static_cast<LONG>(dim.x),
		.bottom = static_cast<LONG>(dim.y),
	};
	m_context_->set_viewports(cmd_list, 1, &viewport);
	m_context_->set_scissor_rects(cmd_list, 1, &scissor_rect);
	m_context_->bind_pipeline(cmd_list, m_pipeline_);

	/**
	* TODO: Bind table to pipeline
	* should follow builder pattern. add table/constant bindings, then backend
	* does lookup for corresponding root signature (auto generated via reflection)
	*/

	const unsigned int offset = 0;
	const unsigned int stride = 6 * sizeof(float);
	m_context_->bind_vertex_buffers(cmd_list, 0, 1, &m_vertex_buffer_, &stride, &offset);
	m_context_->bind_index_buffer(cmd_list, m_index_buffer_, qhenki::gfx::IndexType::UINT32, 0);

	m_context_->draw_indexed(cmd_list, 3, 0, 0);

	// TODO: resource transition
	// m_context_->

	// Close the command list
	m_context_->close_command_list(cmd_list);

	// Submit command list
	m_context_->submit_command_lists(1, &cmd_list, m_graphics_queue_);

	// Present
	m_context_->present(m_swapchain_);

	// TODO: signal fence
	// update frame index
}

void ExampleApp::resize(int width, int height)
{
	
}

void ExampleApp::destroy()
{
	// destroy pipeline then shaders
}

ExampleApp::~ExampleApp()
{
}

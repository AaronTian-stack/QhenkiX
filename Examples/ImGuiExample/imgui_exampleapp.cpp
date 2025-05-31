#include "imgui_exampleapp.h"

#include <imgui.h>
#include <wrl/client.h>

#include "graphics/shared/math_helper.h"

void ImGUIExampleApp::create()
{
	auto shader_model = m_context_->is_compatibility() ? 
		qhenki::gfx::ShaderModel::SM_5_0 : qhenki::gfx::ShaderModel::SM_6_6;

	auto compiler_flags = CompilerInput::NONE;

	// Create shaders at runtime
	CompilerInput vertex_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::VERTEX_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"vs_main",
		.min_shader_model = shader_model,
	};
	THROW_IF_FALSE(m_context_->create_shader_dynamic(nullptr, &m_vertex_shader_, vertex_shader));

	CompilerInput pixel_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::PIXEL_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"ps_main",
		.min_shader_model = shader_model,
	};
	THROW_IF_FALSE(m_context_->create_shader_dynamic(nullptr, &m_pixel_shader_, pixel_shader));

	qhenki::gfx::PipelineLayoutDesc layout_desc{};
	THROW_IF_FALSE(m_context_->create_pipeline_layout(layout_desc, &m_pipeline_layout_));

	// Create GPU heap
	qhenki::gfx::DescriptorHeapDesc heap_desc_GPU
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
		.descriptor_count = 256, // TODO: expose max count to context
	};
	THROW_IF_FALSE(m_context_->create_descriptor_heap(heap_desc_GPU, m_GPU_heap_));

	// Create CPU heap
	qhenki::gfx::DescriptorHeapDesc heap_desc_CPU
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
		.descriptor_count = 256, // CPU heap has no size limit
	};
	THROW_IF_FALSE(m_context_->create_descriptor_heap(heap_desc_CPU, m_CPU_heap_));

	// Create pipeline
	qhenki::gfx::GraphicsPipelineDesc pipeline_desc =
	{
		.num_render_targets = 1,
		.rtv_formats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		.increment_slot = true,
	};
	THROW_IF_FALSE(m_context_->create_pipeline(pipeline_desc, &m_pipeline_, 
			m_vertex_shader_, m_pixel_shader_, &m_pipeline_layout_, nullptr, L"triangle_pipeline"));

	// A graphics queue is already given to the application by the context

	// Allocate Command Pool(s)/Allocator(s) from queue
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		THROW_IF_FALSE(m_context_->create_command_pool(&m_cmd_pools_[i], m_graphics_queue_));
	}

	qhenki::gfx::Buffer vertex_CPU;
	qhenki::gfx::Buffer index_CPU;

	// Create vertex buffer
	constexpr auto vertices = std::array
	{
		Vertex{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		Vertex{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		Vertex{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};
	qhenki::gfx::BufferDesc desc
	{
		.size = vertices.size() * sizeof(Vertex),
		.usage = qhenki::gfx::BufferUsage::VERTEX,
		.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
	};
	THROW_IF_FALSE(m_context_->create_buffer(desc, vertices.data(), &vertex_CPU, L"Interleaved Position/Color Buffer CPU"));

	desc.visibility = qhenki::gfx::BufferVisibility::GPU;
	THROW_IF_FALSE(m_context_->create_buffer(desc, nullptr, &m_vertex_buffer_, L"Interleaved Position/Color Buffer GPU"));

	constexpr auto indices = std::array{ 0u, 1u, 2u };
	qhenki::gfx::BufferDesc index_desc
	{
		.size = indices.size() * sizeof(uint32_t),
		.usage = qhenki::gfx::BufferUsage::INDEX,
		.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
	};
	THROW_IF_FALSE(m_context_->create_buffer(index_desc, indices.data(), &index_CPU, L"Index Buffer CPU"));

	index_desc.visibility = qhenki::gfx::BufferVisibility::GPU;
	THROW_IF_FALSE(m_context_->create_buffer(index_desc, nullptr, &m_index_buffer_, L"Index Buffer GPU"));

	// Schedule copies to GPU buffers / texture
	THROW_IF_FALSE(m_context_->reset_command_pool(&m_cmd_pools_[get_frame_index()]));
	qhenki::gfx::CommandList cmd_list;
	THROW_IF_FALSE(m_context_->create_command_list(&cmd_list, m_cmd_pools_[get_frame_index()]));
	m_context_->copy_buffer(&cmd_list, vertex_CPU, 0, &m_vertex_buffer_, 0, desc.size);
	m_context_->copy_buffer(&cmd_list, index_CPU, 0, &m_index_buffer_, 0, index_desc.size);

	THROW_IF_FALSE(m_context_->close_command_list(&cmd_list));
	auto current_fence_value = ++m_fence_frame_ready_val_[get_frame_index()];
	qhenki::gfx::SubmitInfo info
	{
		.command_list_count = 1,
		.command_lists = &cmd_list,
		.signal_fence_count = 1,
		.signal_fences = &m_fence_frame_ready_,
		.signal_values = &current_fence_value,
	};

	m_context_->submit_command_lists(info, &m_graphics_queue_);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Docking Branch
	m_context_->init_imgui(m_window_, m_swapchain_);

	qhenki::gfx::WaitInfo wait_info
	{
		.count = 1,
		.fences = &m_fence_frame_ready_,
		.values = &m_fence_frame_ready_val_[get_frame_index()]
	};
	THROW_IF_FALSE(m_context_->wait_fences(wait_info)); // Block CPU until done
}

void ImGUIExampleApp::render()
{
	m_context_->start_imgui_frame();
	ImGui::ShowDemoWindow();

	const auto dim = this->m_window_.get_display_size();

	THROW_IF_FALSE(m_context_->reset_command_pool(&m_cmd_pools_[get_frame_index()]));

	// Create a command list in the open state
	qhenki::gfx::CommandList cmd_list;
	THROW_IF_FALSE(m_context_->create_command_list(&cmd_list, m_cmd_pools_[get_frame_index()]));

	// Resource transition
	qhenki::gfx::ImageBarrier barrier_render = 
	{
		.src_stage = qhenki::gfx::SyncStage::SYNC_DRAW, // Ensure we are not drawing anything to swapchain (still might be drawing from previous frame)
		.dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET, // Setting swapchain as render target requires transition to finish first

		.src_access = qhenki::gfx::AccessFlags::ACCESS_COMMON,
		.dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,

		.src_layout = qhenki::gfx::Layout::PRESENT,
		.dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
	};
	m_context_->set_barrier_resource(1, &barrier_render, m_swapchain_, get_frame_index());
	m_context_->issue_barrier(&cmd_list, 1, &barrier_render);

	// Clear back buffer / Start render pass
	m_context_->start_render_pass(&cmd_list, m_swapchain_, nullptr, get_frame_index());

	// Set viewport
	const D3D12_VIEWPORT viewport
	{
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<float>(dim.x),
		.Height = static_cast<float>(dim.y),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	const D3D12_RECT scissor_rect
	{
		.left = 0,
		.top = 0,
		.right = static_cast<LONG>(dim.x),
		.bottom = static_cast<LONG>(dim.y),
	};
	m_context_->set_viewports(&cmd_list, 1, &viewport);
	m_context_->set_scissor_rects(&cmd_list, 1, &scissor_rect);

	m_context_->bind_pipeline_layout(&cmd_list, m_pipeline_layout_);

	m_context_->set_descriptor_heap(&cmd_list, m_GPU_heap_);

	THROW_IF_FALSE(m_context_->bind_pipeline(&cmd_list, m_pipeline_));

	const unsigned int offset = 0;
	constexpr auto stride = static_cast<UINT>(sizeof(Vertex));
	const auto buffers = &m_vertex_buffer_;
	m_context_->bind_vertex_buffers(&cmd_list, 0, 1, &buffers, &stride, &offset);
	m_context_->bind_index_buffer(&cmd_list, m_index_buffer_, qhenki::gfx::IndexType::UINT32, 0);

	m_context_->draw_indexed(&cmd_list, 3, 0, 0);

	// ImGUI Render
	ImGui::Render();
	m_context_->render_imgui_draw_data(&cmd_list);

	// Resource transition
	qhenki::gfx::ImageBarrier barrier_present = 
	{
		.src_stage = qhenki::gfx::SyncStage::SYNC_DRAW, // Wait for all draws to swapchain to finish before transitioning to presentation
		.dst_stage = qhenki::gfx::SyncStage::SYNC_NONE, // No other stages will use swapchain resources

		.src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
		.dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,

		.src_layout = qhenki::gfx::Layout::RENDER_TARGET,
		.dst_layout = qhenki::gfx::Layout::PRESENT,
	};
	m_context_->set_barrier_resource(1, &barrier_present, m_swapchain_, get_frame_index());
	m_context_->issue_barrier(&cmd_list, 1, &barrier_present);

	// Close the command list
	m_context_->close_command_list(&cmd_list);

	// Submit command list
	auto current_fence_value = m_fence_frame_ready_val_[get_frame_index()];
	qhenki::gfx::SubmitInfo info
	{
		.command_list_count = 1,
		.command_lists = &cmd_list,
		.signal_fence_count = 1,
		.signal_fences = &m_fence_frame_ready_,
		.signal_values = &current_fence_value,
	};
	m_context_->submit_command_lists(info, &m_graphics_queue_);

	// You MUST call Present at the end of the render loop
	// TODO: change for Vulkan
	m_context_->present(m_swapchain_, 0, nullptr, get_frame_index());

	increment_frame_index();

	// If next frame is ready to be used, otherwise wait
	if (m_context_->get_fence_value(m_fence_frame_ready_) < m_fence_frame_ready_val_[get_frame_index()])
	{
		qhenki::gfx::WaitInfo wait_info
		{
			.wait_all = true,
			.count = 1,
			.fences = &m_fence_frame_ready_,
			.values = &current_fence_value,
			.timeout = INFINITE
		};
		m_context_->wait_fences(wait_info);
	}
	m_fence_frame_ready_val_[get_frame_index()] = current_fence_value + 1;
}

void ImGUIExampleApp::resize(int width, int height)
{
}

void ImGUIExampleApp::destroy()
{
	m_context_->destroy_imgui();
}

ImGUIExampleApp::~ImGUIExampleApp()
{
}

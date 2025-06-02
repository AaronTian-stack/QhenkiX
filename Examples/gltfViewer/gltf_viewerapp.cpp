#include "gltf_viewerapp.h"

#include <wrl/client.h>

#include "imgui.h"
#include "graphics/shared/math_helper.h"

#include <SDL3/SDL_dialog.h>

void gltfViewerApp::create()
{
	auto shader_model = m_context_->is_compatibility() ? 
		qhenki::gfx::ShaderModel::SM_5_0 : qhenki::gfx::ShaderModel::SM_6_6;

	auto compiler_flags = CompilerInput::NONE;

	std::vector<std::wstring> defines;
	defines.reserve(1);
	if (m_context_->is_compatibility())
	{
		defines.push_back(L"DX11");
	}
	else
	{
		defines.push_back(L"DX12");
	}

	// Create shaders at runtime
	CompilerInput vertex_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::VERTEX_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"vs_main",
		.min_shader_model = shader_model,
		.defines = defines,
	};
	THROW_IF_FALSE(m_context_->create_shader_dynamic(nullptr, &m_vertex_shader_, vertex_shader));

	CompilerInput pixel_shader =
	{
		.flags = compiler_flags,
		.shader_type = qhenki::gfx::ShaderType::PIXEL_SHADER,
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"ps_main",
		.min_shader_model = shader_model,
		.defines = defines,
	};
	THROW_IF_FALSE(m_context_->create_shader_dynamic(nullptr, &m_pixel_shader_, pixel_shader));

	// Create pipeline layout
	qhenki::gfx::LayoutBinding b1 // Constant buffer for camera matrix
	{
		.binding = 0,
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	};
	qhenki::gfx::LayoutBinding b2 // SRV for texture
	{
		.binding = 1, // TODO: figure out how to handle this for Vulkan
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	};
	qhenki::gfx::LayoutBinding b3 // Sampler for texture
	{
		.binding = 0,
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
	};
	qhenki::gfx::PipelineLayoutDesc layout_desc{};
	layout_desc.spaces[0] = { b1, b2 };
	layout_desc.spaces[1] = { b3 }; // Samplers need their own space/table
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

	qhenki::gfx::DescriptorHeapDesc dsv_heap_desc
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::DSV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
		.descriptor_count = 256,
	};
	THROW_IF_FALSE(m_context_->create_descriptor_heap(dsv_heap_desc, m_dsv_heap_));

	// Create Sampler Heap
	qhenki::gfx::DescriptorHeapDesc sampler_heap_desc
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU, // Create samplers directly on GPU heap
		.descriptor_count = 16, // TODO: expose max count to context
	};
	THROW_IF_FALSE(m_context_->create_descriptor_heap(sampler_heap_desc, m_sampler_heap_));

	// Create pipeline
	qhenki::gfx::GraphicsPipelineDesc pipeline_desc =
	{
		.depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
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
		THROW_IF_FALSE(m_context_->create_command_pool(&m_cmd_pools_thread_[i], m_graphics_queue_));
	}

	// Depth buffer
	const auto display_size = m_window_.get_display_size();
	qhenki::gfx::TextureDesc depth_desc
	{
		.width = display_size.x,
		.height = display_size.y,
		.format = DXGI_FORMAT_D32_FLOAT,
		.dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
		.initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
	};
	THROW_IF_FALSE(m_context_->create_texture(depth_desc, &m_depth_buffer_, L"Depth Buffer Texture"));
	m_context_->create_descriptor_depth_stencil(m_depth_buffer_, m_dsv_heap_, &m_depth_buffer_descriptor_);

	// Make 2 matrix constant buffers for double buffering
	qhenki::gfx::BufferDesc matrix_desc
	{
		.size = MathHelper::align_u32(sizeof(qhenki::CameraMatrices), CONSTANT_BUFFER_ALIGNMENT),
		.usage = qhenki::gfx::BufferUsage::UNIFORM,
		.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
	};
	// TODO: persistent mapping flag
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		THROW_IF_FALSE(m_context_->create_buffer(matrix_desc, nullptr, &m_matrix_buffers_[i], L"Matrix Buffer"));
		THROW_IF_FALSE(m_context_->create_descriptor(m_matrix_buffers_[i], m_CPU_heap_, 
			&m_matrix_descriptors_[i], qhenki::gfx::BufferDescriptorType::CBV));
	}

	// Create sampler
	qhenki::gfx::SamplerDesc sampler_desc
	{
		.min_filter = qhenki::gfx::Filter::NEAREST,
		.mag_filter = qhenki::gfx::Filter::NEAREST,
	}; // Default parameters
	THROW_IF_FALSE(m_context_->create_sampler(sampler_desc, &m_sampler_));
	// Create sampler descriptor
	THROW_IF_FALSE(m_context_->create_descriptor(m_sampler_, m_sampler_heap_, &m_sampler_descriptor_));
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Docking Branch
	m_context_->init_imgui(m_window_, m_swapchain_);

	m_camera_controller_.set_camera(&m_camera_.transform_);
}

struct ContextModel
{
	qhenki::gfx::Context* context;
	qhenki::gfx::CommandPool* pool;
	qhenki::gfx::Queue* queue;
	GLTFModel* model;
	std::mutex* mutex;
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	GLTFLoader loader;
	
	auto context_model = static_cast<ContextModel*>(userdata);
	assert(context_model);

	ContextData context_data
	{
		.context = context_model->context,
		.pool = context_model->pool,
		.queue = context_model->queue,
	};

	std::scoped_lock lock(*context_model->mutex);
	loader.load(*filelist, context_model->model, context_data);
}

void gltfViewerApp::render()
{
	m_context_->start_imgui_frame();
	{
		//ImGui::ShowDemoWindow();
		const float PAD = 10.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos;
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos;
		window_pos.x = work_pos.x + work_size.x - PAD;
		window_pos.y = work_pos.y + PAD;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, { 1.0f, 0.0f });
		ImGui::SetNextWindowBgAlpha(0.5f);
		if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		}

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::MenuItem("Quit"))
			{
				m_QUIT_ = true;
			}
			if (ImGui::MenuItem("Load File"))
			{
				static const SDL_DialogFileFilter filters[] =
				{
					{ "glTF files (*.gltf;*glb)",  "gltf;glb" },
				};
				static ContextModel cm
				{
					.context = m_context_.get(),
					.pool = &m_cmd_pools_thread_[get_frame_index()], // Needs its own command pool for async loading
					.queue = &m_graphics_queue_,
					.model = &m_model_,
					.mutex = &m_model_mutex_
				};
				SDL_ShowOpenFileDialog(callback, &cm, m_window_.get_window(), filters, SDL_arraysize(filters), nullptr, false);
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::End();
	}

	const auto dim = this->m_window_.get_display_size();
	m_camera_.viewport_width = static_cast<float>(dim.x);
	m_camera_.viewport_height = static_cast<float>(dim.y);

	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse)
	{
		constexpr auto speed = 0.01f;
		const auto delta = m_input_manager_.get_mouse_delta();
		bool left = m_input_manager_.is_mouse_button_down(SDL_BUTTON_LEFT);
		bool right = m_input_manager_.is_mouse_button_down(SDL_BUTTON_RIGHT);
		SDL_SetWindowRelativeMouseMode(m_window_.get_window(), left || right);
		if (left)
		{
			m_camera_controller_.rotate(delta.x * speed, delta.y * speed);
		}
		if (right)
		{
			m_camera_controller_.translate(-delta.x * speed, delta.y * speed);
		}
		if (m_input_manager_.get_mouse_scroll().y != 0)
		{
			auto desired_distance = m_camera_controller_.get_target_distance() +
				m_input_manager_.get_mouse_scroll().y * 0.2f;
			m_camera_controller_.set_target_distance(desired_distance);
		}
	}

	m_camera_.update(false);

	// Update matrix buffer
	const auto buffer_pointer = m_context_->map_buffer(m_matrix_buffers_[get_frame_index()]);
	assert(buffer_pointer);
	memcpy(buffer_pointer, &m_camera_.matrices, sizeof(qhenki::CameraMatrices));
	m_context_->unmap_buffer(m_matrix_buffers_[get_frame_index()]);

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
	std::array barriers = { &barrier_render };
	m_context_->set_barrier_resource(1, barriers.data(), m_swapchain_, get_frame_index());
	m_context_->issue_barrier(&cmd_list, 1, barriers.data());

	// Clear back buffer / Start render pass
	std::array clear_values = { 0.f, 0.f, 0.f, 1.f };
	qhenki::gfx::RenderTarget depth
	{
		.clear_params = {
			.dsv_clear_params = { 1.f, 0 }
		},
		.clear_type = qhenki::gfx::RenderTarget::Depth,
		.descriptor = m_depth_buffer_descriptor_,
	};
	m_context_->start_render_pass(&cmd_list, m_swapchain_, clear_values.data(), &depth, get_frame_index());

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

	m_context_->set_descriptor_heap(&cmd_list, m_GPU_heap_, m_sampler_heap_);

	THROW_IF_FALSE(m_context_->bind_pipeline(&cmd_list, m_pipeline_));

	// Bind resources
	if (m_context_->is_compatibility())
	{
		std::array mbs = { &m_matrix_buffers_[get_frame_index()] };
		m_context_->compatibility_set_constant_buffers(0, 1,
		                                               mbs.data(), qhenki::gfx::PipelineStage::VERTEX);
		std::array samps = { &m_sampler_ };
		m_context_->compatibility_set_samplers(0, 1, samps.data(), qhenki::gfx::PipelineStage::PIXEL);
	}
	else
	{
		qhenki::gfx::Descriptor descriptor; // Location of start of GPU heap
		THROW_IF_FALSE(m_context_->get_descriptor(0, m_GPU_heap_, &descriptor));

		// Parameter 0 is table, set to start at beginning of GPU heap
		m_context_->set_descriptor_table(&cmd_list, 0, descriptor);

		// Copy matrix and texture descriptors to GPU heap
		THROW_IF_FALSE(m_context_->copy_descriptors(1, m_matrix_descriptors_[get_frame_index()], descriptor));
		THROW_IF_FALSE(m_context_->get_descriptor(1, m_GPU_heap_, &descriptor)); // 1
	}

	{ // Render
		// Draw glTF model
		std::unique_lock<std::mutex> lock(m_model_mutex_, std::defer_lock);
		if (lock.try_lock() && m_model_.root_node >= 0) // If not still loading (because it is async function)
		{
			static std::vector<int> stack;
			stack.reserve(m_model_.nodes.size());
			stack.push_back(m_model_.root_node);
			while (!stack.empty())
			{
				const auto current_node_index = stack.back();
				const auto& current_node = m_model_.nodes[current_node_index];
				stack.pop_back();

				// TODO Calculate node global transform

				// Draw the node
				const auto& mesh = m_model_.meshes[current_node.mesh_index];
				for (const auto& prim : mesh.primitives)
				{
					for (const auto& attr : prim.attributes)
					{
						if (attribute_to_slot.contains(attr.name))
						{
							const auto slot = attribute_to_slot.at(attr.name);

							const auto& accessor = m_model_.accessors[attr.accessor_index];
							const auto& buffer_view = m_model_.buffer_views[accessor.buffer_view];
							
							auto buffer = &m_model_.buffers[buffer_view.buffer_index];

							assert(buffer_view.stride <= std::numeric_limits<unsigned>::max());
							assert(accessor.offset <= std::numeric_limits<unsigned>::max());
							unsigned stride = buffer_view.stride;

							assert(accessor.type == TINYGLTF_TYPE_SCALAR || accessor.type == TINYGLTF_TYPE_VEC2 || accessor.type == TINYGLTF_TYPE_VEC3);
							unsigned offset = accessor.offset + buffer_view.offset;

							m_context_->bind_vertex_buffers(&cmd_list, slot, 1, &buffer, &stride, &offset);
						}
					}
					const auto& index_accessor = m_model_.accessors[prim.indices];
					const auto& buffer_view = m_model_.buffer_views[index_accessor.buffer_view];
					const auto& index_buffer = m_model_.buffers[buffer_view.buffer_index];
					auto index_type = index_accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT 
						? qhenki::gfx::IndexType::UINT16 : qhenki::gfx::IndexType::UINT32;

					m_context_->bind_index_buffer(&cmd_list, index_buffer, index_type, index_accessor.offset + buffer_view.offset);
					// Draw
					m_context_->draw_indexed(&cmd_list, index_accessor.count, 0, 0);
				}

				// Add the children to the stack
				for (const auto& child : m_model_.nodes[current_node_index].children_indices)
				{
					stack.push_back(child);
				}
			}
			lock.unlock();
		}
	}

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
	barriers = { &barrier_present };
	m_context_->set_barrier_resource(1, barriers.data(), m_swapchain_, get_frame_index());
	m_context_->issue_barrier(&cmd_list, 1, barriers.data());

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

void gltfViewerApp::resize(int width, int height)
{
	// TODO: resize depth buffer
}

void gltfViewerApp::destroy()
{
	m_context_->destroy_imgui();
}

gltfViewerApp::~gltfViewerApp()
{
}

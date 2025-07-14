#include "gltf_viewerapp.h"

#include <wrl/client.h>

#include <imgui/imgui.h>
#include <qhenkiX/helper/math_helper.h>

#include <SDL3/SDL_dialog.h>

#include "qhenkiX/helper/general_helper.h"

void gltfViewerApp::update_global_transform(GLTFModel& model, GLTFModel::Node& node)
{
	if (node.parent_index > 0)
	{
		if (model.nodes[node.parent_index].global_transform.dirty)
		{
			update_global_transform(model, model.nodes[node.parent_index]);
		}
		// Pre-multiply local transform with parent's global transform
		// TODO: stop store load every time
		node.global_transform.transform = model.nodes[node.parent_index].global_transform.transform * node.local_transform;
	}
	else
	{
		node.global_transform.transform = node.local_transform;
	}
}

void gltfViewerApp::create()
{
	auto shader_model = m_context->is_compatibility() ? 
		qhenki::gfx::ShaderModel::SM_5_0 : qhenki::gfx::ShaderModel::SM_6_6;

	std::vector<std::wstring> defines;
	defines.reserve(1);
	if (m_context->is_compatibility())
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
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"vs_main",
		.defines = defines,
		.min_shader_model = shader_model,
		.shader_type = qhenki::gfx::ShaderType::VERTEX_SHADER,
		//.flags = CompilerInput::DEBUG,
	};
	THROW_IF_FALSE(m_context->create_shader_dynamic(nullptr, &m_vertex_shader, vertex_shader));

	CompilerInput pixel_shader =
	{
		.path = L"base-shaders/BaseShader.hlsl",
		.entry_point = L"ps_main",
		.defines = defines,
		.min_shader_model = shader_model,
		.shader_type = qhenki::gfx::ShaderType::PIXEL_SHADER,
		//.flags = CompilerInput::DEBUG,
	};
	THROW_IF_FALSE(m_context->create_shader_dynamic(nullptr, &m_pixel_shader, pixel_shader));

	// Create pipeline layout
	qhenki::gfx::LayoutBinding camera // Constant buffers
	{
		.binding = 0,
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	};
	qhenki::gfx::LayoutBinding material
	{
		.binding = 1,
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	};
	qhenki::gfx::LayoutBinding textures // SRV for texture
	{
		.binding = 2, // TODO: figure out how to handle this for Vulkan
		.count = 1000,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	};
	qhenki::gfx::LayoutBinding samplers // Sampler for texture
	{
		.binding = 0,
		.count = 1,
		.type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
	};
	qhenki::gfx::PipelineLayoutDesc layout_desc{};
	layout_desc.spaces[0] = { camera, material, textures };
	layout_desc.spaces[1] = { samplers }; // Samplers need their own space/table
	layout_desc.push_ranges.push_back(
		qhenki::gfx::PushRange
		{
			.size = sizeof(XMFLOAT4X4) * 2 + sizeof(int),
			.binding = 0,
		}
	);
	THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

	// Create GPU heap
	qhenki::gfx::DescriptorHeapDesc heap_desc_GPU
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
		.descriptor_count = 256, // TODO: expose max count to context
	};
	THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap, L"GPU heap"));

	// Create CPU heap
	qhenki::gfx::DescriptorHeapDesc heap_desc_CPU
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
		.descriptor_count = 256, // CPU heap has no size limit
	};
	THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap, L"CPU heap"));

	qhenki::gfx::DescriptorHeapDesc dsv_heap_desc
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::DSV,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
		.descriptor_count = 256,
	};
	THROW_IF_FALSE(m_context->create_descriptor_heap(dsv_heap_desc, &m_dsv_heap, L"DSV heap"));

	// Create Sampler Heap
	qhenki::gfx::DescriptorHeapDesc sampler_heap_desc
	{
		.type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
		.visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU, // Create samplers directly on GPU heap
		.descriptor_count = 16, // TODO: expose max count to context
	};
	THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap, L"Sampler heap"));

	// Create pipeline
	qhenki::gfx::GraphicsPipelineDesc pipeline_desc =
	{
		.depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
		.num_render_targets = 1,
		.rtv_formats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		.dsv_format = DXGI_FORMAT_D32_FLOAT,
		.increment_slot = true,
	};
	THROW_IF_FALSE(m_context->create_pipeline(pipeline_desc, &m_pipeline, 
				m_vertex_shader, m_pixel_shader, &m_pipeline_layout, L"Triangle pipeline"));

	// A graphics queue is already given to the application by the context

	// Allocate Command Pool(s)/Allocator(s) from queue
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], m_graphics_queue));
		THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools_thread[i], m_graphics_queue));
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
	THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, L"Depth Buffer Texture"));
	THROW_IF_FALSE(m_context->create_descriptor_depth_stencil(m_depth_buffer, &m_dsv_heap, &m_depth_buffer_descriptor));

	// Make 2 matrix constant buffers for double buffering
	qhenki::gfx::BufferDesc matrix_desc
	{
		.size = MathHelper::align_u32(sizeof(qhenki::CameraMatrices), CONSTANT_BUFFER_ALIGNMENT),
		.usage = qhenki::gfx::BufferUsage::CONSTANT,
		.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
	};
	// TODO: persistent mapping flag
	for (int i = 0; i < m_frames_in_flight; i++)
	{
		THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], L"Matrix Buffer"));
		THROW_IF_FALSE(m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap,
					&m_matrix_descriptors[i]));
	}

	if (m_context->is_compatibility())
	{
		qhenki::gfx::BufferDesc desc
		{
			.size = MathHelper::align_u32(sizeof(XMFLOAT4X4) * 2 + sizeof(int), CONSTANT_BUFFER_ALIGNMENT),
			.usage = qhenki::gfx::BufferUsage::CONSTANT,
			.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
		};
		THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_model_buffer, L"Model Buffer"));
		THROW_IF_FALSE(m_context->create_descriptor_constant_view(m_model_buffer, &m_CPU_heap, &m_model_descriptor));
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Docking Branch
	m_context->init_imgui(m_window_, m_swapchain);

	m_camera_controller.set_camera(&m_camera.transform);
}

struct ContextModel
{
	qhenki::gfx::Context* context;
	qhenki::gfx::CommandPool* pool;
	qhenki::gfx::Queue* queue;
	size_t model_count;
	GLTFModel* models;
	std::mutex* mutex;
	std::atomic_int* model_index_to_load_into;

	struct HeapAndList
	{
		qhenki::gfx::DescriptorHeap* heap; // Heap to use to create descriptors
		std::vector<qhenki::gfx::Descriptor>* descriptors; // (in/out) Descriptors to initialize
	};
	struct HeapAndDescriptor
	{
		qhenki::gfx::DescriptorHeap* heap; // Heap to use to create descriptor
		qhenki::gfx::Descriptor* descriptor; // (in/out) Descriptor to initialize
	};
	HeapAndList texture;
	HeapAndList sampler;
	HeapAndDescriptor gltfTexture;
	HeapAndDescriptor material;
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist || !*filelist)
	{
		return;
	}

	GLTFLoader loader;

	const auto context_model = static_cast<ContextModel*>(userdata);
	assert(context_model);

	ContextData context_data
	{
		.context = context_model->context,
		.pool = context_model->pool,
		.queue = context_model->queue,
	};

	auto& context = *context_model->context;

	std::scoped_lock lock(*context_model->mutex);

	const auto result = loader.load(*filelist, &context_model->models[*context_model->model_index_to_load_into], context_data);
	if (result)
	{
		printf("glTF model loaded: %s\n", *filelist);
	}
	else
	{
		printf("FAILED to load glTF model: %s\n", *filelist);
	}

	// Only copy below descriptors when model lock is held so is ok.
	// Create views
    {
        auto& model = context_model->models[*context_model->model_index_to_load_into];

        auto& matHeap = *context_model->material.heap;

        auto& texDescriptors = *context_model->texture.descriptors;
        auto& texHeap = *context_model->texture.heap;

        auto& sampDescriptors = *context_model->sampler.descriptors;
        auto& sampHeap = *context_model->sampler.heap;

		// glTF Texture Descriptor
		assert(matHeap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
		context.create_descriptor_shader_view(model.texture_buffer, &matHeap, context_model->gltfTexture.descriptor);

        // Material buffer Descriptor
        assert(matHeap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
        context.create_descriptor_shader_view(model.material_buffer, &matHeap, context_model->material.descriptor);

        // Texture Descriptors
		if (model.images.size() > texDescriptors.size())
		{
			texDescriptors.resize(model.images.size());
		}
        for (size_t i = 0; i < model.images.size(); i++)
        {
            assert(texHeap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
            // TODO: ensure range is contiguous if descriptors already exist
            context.create_descriptor_shader_view(model.images[i], &texHeap, &texDescriptors[i]);
        }

        // Sampler Descriptors
		if (model.samplers.size() > sampDescriptors.size())
		{
			sampDescriptors.resize(model.samplers.size());
		}
        for (size_t i = 0; i < model.samplers.size(); i++)
        {
            // Should just be created from start of heap
            context.create_descriptor(model.samplers[i], &sampHeap, &sampDescriptors[i]);
        }
    }
	*context_model->model_index_to_load_into = (*context_model->model_index_to_load_into + 1) % context_model->model_count; // Toggle model index
}

void gltfViewerApp::render()
{
	m_context->start_imgui_frame();
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
			// Display frametime graph
			constexpr size_t max_frames = 100;
			static float frame_times[max_frames];
			static size_t frame_index = 0;
			static bool buffer_filled = false;

			// Record new frame time
			frame_times[frame_index] = ImGui::GetIO().DeltaTime;
			frame_index = (frame_index + 1) % max_frames;
			if (frame_index == 0) buffer_filled = true;

			static float ordered_times[max_frames];
			size_t count = buffer_filled ? max_frames : frame_index;

			for (size_t i = 0; i < count; ++i) 
			{
				size_t index = (frame_index + i) % max_frames;
				ordered_times[i] = frame_times[index];
			}

			ImGui::PlotLines("##plot", ordered_times, max_frames, 0, "",
				0.f, 0.05f, ImVec2(ImGui::GetContentRegionAvail().x, 40));
		}

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::MenuItem("Quit"))
			{
				m_QUIT = true;
			}
			if (ImGui::MenuItem("Load File"))
			{
				static const SDL_DialogFileFilter filters[] =
				{
					{ "glTF files (*.gltf;*glb)",  "gltf;glb" },
				};

				/*
				 * The render thread will attempt to use model at m_model_index.
				 * This will not cause issues unless model at m_model_index is being used in frame in flight,
				 * which is impossible because you would have had to call this function twice in the same frame.
				 */

				thread_local ContextModel cm;
				cm =
				{
					.context = m_context.get(),
					.pool = &m_cmd_pools_thread[get_frame_index()], // Needs its own command pool for async loading
					.queue = &m_graphics_queue,
					.model_count = m_models.size(),
					.models = m_models.data(),
					.mutex = &m_model_mutex,
					.model_index_to_load_into = &m_model_index_to_load_into,
					.texture
					{
						.heap = &m_CPU_heap, // Use same CPU heap as matrix descriptors
						.descriptors = &m_model_texture_descriptors,
					},
					.sampler
					{
						.heap = &m_sampler_heap,
						.descriptors = &m_sampler_descriptors,
					},
					.gltfTexture =
					{
						.heap = &m_CPU_heap, // Use same CPU heap as matrix descriptors
						.descriptor = &m_model_gltfTexture_descriptor,
					},
					.material =
					{
						.heap = &m_CPU_heap, // Use same CPU heap as matrix descriptors
						.descriptor = &m_model_material_descriptor,
					},
				};
				SDL_ShowOpenFileDialog(callback, &cm, m_window_.get_window(), filters, SDL_arraysize(filters), nullptr, false);
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::End();
	}

	const auto dim = this->m_window_.get_display_size();
	m_camera.viewport_width = static_cast<float>(dim.x);
	m_camera.viewport_height = static_cast<float>(dim.y);

	if (ImGuiIO& io = ImGui::GetIO(); !io.WantCaptureMouse)
	{
		auto speed = 0.01f;
		const auto delta = m_input_manager.get_mouse_delta();
		bool left = m_input_manager.is_mouse_button_down(SDL_BUTTON_LEFT);
		bool right = m_input_manager.is_mouse_button_down(SDL_BUTTON_RIGHT);
		SDL_SetWindowRelativeMouseMode(m_window_.get_window(), left || right);
		if (left)
		{
			m_camera_controller.rotate(delta.x * speed, delta.y * speed);
		}
		if (right)
		{
			if (m_input_manager.is_key_down(SDL_SCANCODE_LSHIFT) || m_input_manager.is_key_down(SDL_SCANCODE_RSHIFT))
			{
				speed *= 100.f;
			}
			m_camera_controller.translate(-delta.x * speed, delta.y * speed);
		}
		bool middle = m_input_manager.is_mouse_button_down(SDL_BUTTON_MIDDLE);
		bool vertical_scrolling = m_input_manager.get_mouse_scroll().y != 0;
		if ((vertical_scrolling != 0) ^ middle)
		{
			float amount = 0.f;
			if (vertical_scrolling)
			{
				amount = m_input_manager.get_mouse_scroll().y * 0.2f;
			}
			if (middle)
			{
				amount = -delta.y;
			}
			auto desired_distance = m_camera_controller.get_target_distance() + amount;
			m_camera_controller.set_target_distance(desired_distance);
		}
	}

	m_camera.update(false);

	// Update matrix buffer
	const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[get_frame_index()]);
	assert(buffer_pointer);
	memcpy(buffer_pointer, &m_camera.matrices, sizeof(qhenki::CameraMatrices));
	memcpy(static_cast<uint8_t*>(buffer_pointer) + sizeof(qhenki::CameraMatrices), &m_camera.transform.translation, sizeof(XMFLOAT3));
	m_context->unmap_buffer(m_matrix_buffers[get_frame_index()]);

	THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[get_frame_index()]));

	// Create a command list in the open state
	qhenki::gfx::CommandList cmd_list;
	THROW_IF_FALSE(m_context->create_command_list(&cmd_list, m_cmd_pools[get_frame_index()], L"main command list"));

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
	m_context->set_barrier_resource(1, &barrier_render, m_swapchain, get_frame_index());
	m_context->issue_barrier(&cmd_list, 1, &barrier_render);

	// Clear back buffer / Start render pass
	std::array clear_values = { 0.f, 0.f, 0.f, 1.f };
	qhenki::gfx::RenderTarget depth
	{
		.clear_params = {
			.dsv_clear_params = { 1.f, 0 }
		},
		.clear_type = qhenki::gfx::RenderTarget::Depth,
		.descriptor = m_depth_buffer_descriptor,
	};
	m_context->start_render_pass(&cmd_list, &m_swapchain, clear_values.data(), &depth, get_frame_index());

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
	m_context->set_viewports(&cmd_list, 1, &viewport);
	m_context->set_scissor_rects(&cmd_list, 1, &scissor_rect);

	m_context->bind_pipeline_layout(&cmd_list, m_pipeline_layout);

	m_context->set_descriptor_heap(&cmd_list, m_GPU_heap, m_sampler_heap);

	THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_pipeline));

	// Bind resources
	if (m_context->is_compatibility())
	{
		m_context->compatibility_set_constant_buffers(0, 1, qhenki::util::ptr_array(m_matrix_buffers[get_frame_index()]).data(), qhenki::gfx::PipelineStage::VERTEX);
	}
	else
	{
		qhenki::gfx::Descriptor descriptor; // Location of start of GPU heap
		THROW_IF_FALSE(m_context->get_descriptor(0, &m_GPU_heap, &descriptor));

		// Parameter 1 is table, set to start at beginning of GPU heap
		m_context->set_descriptor_table(&cmd_list, 1, descriptor);

		// Copy matrix descriptors to GPU heap
		THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[get_frame_index()], descriptor));

		// Sampler
		THROW_IF_FALSE(m_context->get_descriptor(0, &m_sampler_heap, &descriptor));
		m_context->set_descriptor_table(&cmd_list, 2, descriptor);
	}

	{ // Render
		// Draw glTF model
		std::unique_lock lock(m_model_mutex, std::defer_lock);
		const auto model_to_render = m_model_index_to_load_into > 0 ? m_model_index_to_load_into - 1 : m_models.size() - 1;
		auto& m_model = m_models[model_to_render];
		if (lock.try_lock() && m_model.root_node >= 0) // If not still loading (because it is async function)
		{
			// Bindless bind textures, only need to do before all draws
			if (!m_context->is_compatibility()) // NOT compatibility
			{
				qhenki::gfx::Descriptor descriptor;

				// Make sure these match in the shader
				int start = 1;

				// Copy texture descriptor
				THROW_IF_FALSE(m_context->get_descriptor(start++, &m_GPU_heap, &descriptor));
				THROW_IF_FALSE(m_context->copy_descriptors(1, m_model_gltfTexture_descriptor, descriptor));

				// Copy material descriptor
				THROW_IF_FALSE(m_context->get_descriptor(start++, &m_GPU_heap, &descriptor));
				THROW_IF_FALSE(m_context->copy_descriptors(1, m_model_material_descriptor, descriptor));

				// Copy texture descriptors
				THROW_IF_FALSE(m_context->get_descriptor(start++, &m_GPU_heap, &descriptor));
				// TODO: check that CPU texture descriptors are contiguous, otherwise singular copy will not work
				// Use the textures size, not descriptors because descriptors may be larger than texture count due to left over from old model
				THROW_IF_FALSE(m_context->copy_descriptors(m_model.textures.size(), m_model_texture_descriptors[0], descriptor));
			}
			// Compatibility will bind per draw call because we cannot bind all textures at once

			for (int i = 0; i < m_model.nodes.size(); i++)
			{
				auto& current_node = m_model.nodes[i];

				// Skip nodes without valid meshes
				if (current_node.mesh_index < 0 || current_node.mesh_index >= m_model.meshes.size())
				{
					continue;
				}

				// Recalculate node global transform if needed (recursive)
				update_global_transform(m_model, current_node);

				XMFLOAT4X4 global_4x4;
				XMFLOAT4X4 global_4x4_inverse;
				{
					auto m = current_node.global_transform.transform.to_matrix_simd();
					XMStoreFloat4x4(&global_4x4, XMMatrixTranspose(m));
					XMStoreFloat4x4(&global_4x4_inverse, XMMatrixTranspose(XMMatrixInverse(nullptr, m)));
				}

				// Draw the node
				const auto& mesh = m_model.meshes[current_node.mesh_index];
				for (const auto& prim : mesh.primitives)
				{
					for (const auto& attr : prim.attributes)
					{
						if (m_attribute_to_slot.contains(attr.name))
						{
							const auto slot = m_attribute_to_slot.at(attr.name);

							const auto& accessor = m_model.accessors[attr.accessor_index];

							assert(accessor.component_type == TINYGLTF_PARAMETER_TYPE_FLOAT ||
								accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
								accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT);
							assert(accessor.type == TINYGLTF_TYPE_SCALAR || accessor.type == TINYGLTF_TYPE_VEC2 || accessor.type == TINYGLTF_TYPE_VEC3);

							const auto& buffer_view = m_model.buffer_views[accessor.buffer_view];
							
							auto buffer = &m_model.buffers[buffer_view.buffer_index];

							assert(buffer_view.stride <= std::numeric_limits<UINT>::max());
							assert(accessor.offset <= std::numeric_limits<UINT>::max());
							UINT stride = buffer_view.stride;
							if (stride == 0)
							{
								// Tightly packed, infer stride from size of type times number of components
								auto calc_component_size = [](int component_type)
								{
									switch (component_type)
									{
										default:
										case TINYGLTF_PARAMETER_TYPE_FLOAT:
											return sizeof(float);
										case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
											return sizeof(uint16_t);
										case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
											return sizeof(uint32_t);
									}
								};
								auto calc_type_count = [](int type)
								{
									switch (type)
									{
										case TINYGLTF_TYPE_SCALAR:
											return 1;
										case TINYGLTF_TYPE_VEC2:
											return 2;
										default:
										case TINYGLTF_TYPE_VEC3:
											return 3;
									}
								};
								stride = calc_component_size(accessor.component_type) * calc_type_count(accessor.type);
							}

							UINT offset = accessor.offset + buffer_view.offset;

							const UINT length = buffer_view.length;

							m_context->bind_vertex_buffers(&cmd_list, slot, 1, &buffer, &length, &stride, &offset);
						}
					}
					const auto& index_accessor = m_model.accessors[prim.indices];
					const auto& buffer_view = m_model.buffer_views[index_accessor.buffer_view];
					
					const auto& index_buffer = m_model.buffers[buffer_view.buffer_index];
					
					auto index_type = index_accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT 
						? qhenki::gfx::IndexType::UINT16 : qhenki::gfx::IndexType::UINT32;

					UINT index_offset = index_accessor.offset + buffer_view.offset;

					m_context->bind_index_buffer(&cmd_list, index_buffer, index_type, index_offset);

					if (m_context->is_compatibility())
					{
						const auto p = m_context->map_buffer(m_model_buffer);
						memcpy(p, &global_4x4, sizeof(XMFLOAT4X4));
						memcpy(static_cast<uint8_t*>(p) + sizeof(XMFLOAT4X4), &global_4x4_inverse, sizeof(XMFLOAT4X4));
						memcpy(static_cast<uint8_t*>(p) + sizeof(XMFLOAT4X4) * 2, &prim.material_index, sizeof(int));
						m_context->unmap_buffer(m_model_buffer);

						m_context->compatibility_set_constant_buffers(1, 1, qhenki::util::ptr_array(m_model_buffer).data(), qhenki::gfx::PipelineStage::VERTEX);
						m_context->compatibility_set_constant_buffers(1, 1, qhenki::util::ptr_array(m_model_buffer).data(), qhenki::gfx::PipelineStage::PIXEL);

						// Bind based off current material
						const auto& material = m_model.materials[prim.material_index];
						auto set_texture_if_valid = [&](int slot, int index)
						{
							if (index >= 0 && index < static_cast<int>(m_model_texture_descriptors.size())) 
							{
								m_context->compatibility_set_textures(slot, 1, qhenki::util::ptr_array(m_model_texture_descriptors[index]).data(),
								                                      qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE, qhenki::gfx::PipelineStage::PIXEL);
							}
						};
						// 5 textures
						auto ret_t_index = [&](int index)
						{
							if (index < 0)
							{
								return -1;
							}
							return m_model.textures[index].image_index;
						};
						auto start_slot = 3;
						set_texture_if_valid(start_slot++, ret_t_index(material.base_color.index));
						set_texture_if_valid(start_slot++, ret_t_index(material.metallic_roughness.index));
						set_texture_if_valid(start_slot++, ret_t_index(material.normal.index));
						set_texture_if_valid(start_slot++, ret_t_index(material.occlusion.index));
						set_texture_if_valid(start_slot++, ret_t_index(material.emissive.index));

						// Sampler
						auto sampler = [&](int slot, int index)
						{
							if (index < 0)
							{
								// Bind dummy sampler to silence validation warning
								// TODO: pass null instead after refactor
								m_context->compatibility_set_samplers(slot, 1, qhenki::util::ptr_array(m_model.samplers[0]).data(), qhenki::gfx::PipelineStage::PIXEL);
								return;
							}
							const auto sampler_index = m_model.textures[index].sampler_index;
							if (sampler_index < 0)
							{
								qhenki::gfx::Sampler* nul = nullptr;
								m_context->compatibility_set_samplers(slot, 1, &nul, qhenki::gfx::PipelineStage::PIXEL);
							}
							else
							{
								m_context->compatibility_set_samplers(slot, 1, qhenki::util::ptr_array(m_model.samplers[sampler_index]).data(), qhenki::gfx::PipelineStage::PIXEL);
							}
						};
						start_slot = 0;
						sampler(start_slot++, material.base_color.index);
						sampler(start_slot++, material.metallic_roughness.index);
						sampler(start_slot++, material.normal.index);
						sampler(start_slot++, material.occlusion.index);
						sampler(start_slot++, material.emissive.index);

						// We will also bind the material itself here
						m_context->compatibility_set_shader_buffers(2, 1, qhenki::util::ptr_array(m_model_material_descriptor).data(), qhenki::gfx::PipelineStage::PIXEL);
					}
					else
					{
						m_context->set_pipeline_constant(&cmd_list, 0, 0, sizeof(XMFLOAT4X4), &global_4x4);
						m_context->set_pipeline_constant(&cmd_list, 0, sizeof(XMFLOAT4X4), sizeof(XMFLOAT4X4), &global_4x4_inverse);
						int material_index = prim.material_index;
						m_context->set_pipeline_constant(&cmd_list, 0, sizeof(XMFLOAT4X4) * 2, sizeof(int), &material_index);
					}

					// Draw
					m_context->draw_indexed(&cmd_list, index_accessor.count, 0, 0);
				}
			}
		}
	}

	ImGui::Render();
	m_context->render_imgui_draw_data(&cmd_list);

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
	m_context->set_barrier_resource(1, &barrier_present, m_swapchain, get_frame_index());
	m_context->issue_barrier(&cmd_list, 1, &barrier_present);

	// Close the command list
	m_context->close_command_list(&cmd_list);

	// Submit command list
	auto current_fence_value = m_fence_frame_ready_val[get_frame_index()];
	// To delete the model at m_model_index need to wait for this fence value
	//m_model_last_used_fence_value[m_model_index] = current_fence_value;
	qhenki::gfx::SubmitInfo info
	{
		.command_list_count = 1,
		.command_lists = &cmd_list,
		.signal_fence_count = 1,
		.signal_fences = &m_fence_frame_ready,
		.signal_values = &current_fence_value,
	};
	m_context->submit_command_lists(info, &m_graphics_queue);

	// You MUST call Present at the end of the render loop
	// TODO: change for Vulkan
	m_context->present(&m_swapchain, 0, nullptr, get_frame_index());

	increment_frame_index();

	// If next frame is ready to be used, otherwise wait
	if (m_context->get_fence_value(m_fence_frame_ready) < m_fence_frame_ready_val[get_frame_index()])
	{
		qhenki::gfx::WaitInfo wait_info
		{
			.wait_all = true,
			.count = 1,
			.fences = &m_fence_frame_ready,
			.values = &current_fence_value,
			.timeout = INFINITE
		};
		m_context->wait_fences(wait_info);
	}
	m_fence_frame_ready_val[get_frame_index()] = current_fence_value + 1;
}

void gltfViewerApp::resize(int width, int height)
{
	m_context->wait_idle(&m_graphics_queue);
	// Recreate the depth buffer
	qhenki::gfx::TextureDesc depth_desc
	{
		.width = static_cast<uint64_t>(width),
		.height = static_cast<uint32_t>(height),
		.format = DXGI_FORMAT_D32_FLOAT,
		.dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
		.initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
	};
	THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, L"Depth Buffer Texture"));
	// This will recreate the descriptor in place since it already has an offset.
	THROW_IF_FALSE(m_context->create_descriptor_depth_stencil(m_depth_buffer, &m_dsv_heap, &m_depth_buffer_descriptor));
}

void gltfViewerApp::destroy()
{
	m_context->destroy_imgui();
}

gltfViewerApp::~gltfViewerApp()
{
}

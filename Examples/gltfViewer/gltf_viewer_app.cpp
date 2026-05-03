#include "gltf_viewer_app.h"
#include "example_shared/macros.h"
#include "example_shared/shader_loader.h"
#include "example_shared/window_init.h"
#include "shared_structs.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include <imgui/imgui.h>
#include <SDL3/SDL_dialog.h>
#include <tiny_gltf.h>

#include <qhenki/math/transform_simd.h>
#include <qhenki/utility/general_util.h>

namespace
{

void update_global_transform(GLTFModel& model, GLTFModel::Node& node)
{
    if (node.parent_index >= 0)
    {
        if (model.nodes[node.parent_index].global_transform.dirty)
        {
            update_global_transform(model, model.nodes[node.parent_index]);
        }
        const auto parent_simd = qhenki::math::TransformSIMD::load(
            model.nodes[node.parent_index].global_transform.transform);
        const auto local_simd = qhenki::math::TransformSIMD::load(node.local_transform);
        // TODO: Redundant load/store, should instead expand all at once
        (local_simd * parent_simd).store(node.global_transform.transform);
    }
    else
    {
        node.global_transform.transform = node.local_transform;
    }
}
} // namespace

void gltfViewerApp::init_display_window(void* payload)
{
    init_display_window_with_name(*this, m_window, "gltf Viewer", payload);
}

void gltfViewerApp::create()
{
    const auto api = get_graphics_api();
    const bool use_dx11 = api == qhenki::gfx::API::D3D11;

    const char* vs_base_name = use_dx11 ? "base_vs_5_0_vs_main" : "base_vs_6_6_vs_main";
    const char* ps_base_name = use_dx11 ? "base_ps_5_0_ps_main" : "base_ps_6_6_ps_main";
    char vs_name[64]{};
    char ps_name[64]{};
    THROW_IF_FALSE(append_shader_extension(api, vs_base_name, vs_name, sizeof(vs_name)));
    THROW_IF_FALSE(append_shader_extension(api, ps_base_name, ps_name, sizeof(ps_name)));

    uPtr<std::byte, void (*)(void*)> vs_data(nullptr, free);
    qhenki::gfx::Shader vertex_shader;
    read_compiled_shader_bytes(api, vs_name, &vs_data, &vertex_shader.size);
    vertex_shader.data = vs_data.get();

    uPtr<std::byte, void (*)(void*)> ps_data(nullptr, free);
    qhenki::gfx::Shader pixel_shader;
    read_compiled_shader_bytes(api, ps_name, &ps_data, &pixel_shader.size);
    pixel_shader.data = ps_data.get();

    // Create pipeline layout
    qhenki::gfx::LayoutBinding camera // Constant buffers
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::RangeType::CBV,
        };
    qhenki::gfx::LayoutBinding material{
        .binding = 1,
        .count = 1,
        .type = qhenki::gfx::LayoutBinding::RangeType::SRV,
    };
    qhenki::gfx::LayoutBinding textures // SRV for texture
        {
            .binding = 2,
            .count = 1000,
            .type = qhenki::gfx::LayoutBinding::RangeType::SRV,
        };
    qhenki::gfx::LayoutBinding samplers // Sampler for texture
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::RangeType::SAMPLER,
        };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {camera, material, textures};
    layout_desc.spaces[1] = {samplers}; // Samplers need their own space/table
    layout_desc.push_ranges.push_back(qhenki::gfx::PushRange{
        .size = static_cast<uint32_t>(sizeof(ModelConstants)),
        .binding = 0,
        .space = 5,
    });
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    // Create GPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .num_descriptors = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap, "GPU heap"));

    // Create CPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .num_descriptors = heap_desc_GPU.num_descriptors,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap, "CPU heap"));

    // Create Sampler Heap
    qhenki::gfx::DescriptorHeapDesc sampler_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU, // Create samplers directly on GPU heap
        .num_descriptors = 16,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap, "Sampler heap"));

    // Create pipeline
    qhenki::gfx::GraphicsPipelineDesc pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {qhenki::gfx::Format::R8G8B8A8_UNORM},
        .num_render_targets = 1,
        .dsv_format = qhenki::gfx::Format::D32_FLOAT,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(
        pipeline_desc, &m_pipeline, vertex_shader, pixel_shader, &m_pipeline_layout, "Triangle pipeline"));

    // Allocate Command Pool(s)/Allocator(s) from queue
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools_thread[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    // Make 2 matrix constant buffers for double buffering
    qhenki::gfx::BufferDesc matrix_desc{.size = sizeof(CameraData),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    // TODO: persistent mapping flag
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        m_matrix_descriptors[i].offset = i;
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Matrix Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    if (m_context->is_compatibility())
    {
        qhenki::gfx::BufferDesc desc{.size = sizeof(ModelConstants),
                                     .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                     .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
        THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_model_buffer, "Model Buffer"));
        m_model_descriptor.offset = m_frames_in_flight;
        THROW_IF_FALSE(m_context->create_descriptor_constant_view(m_model_buffer, &m_CPU_heap, &m_model_descriptor));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Docking Branch
    m_context->init_imgui(m_window, m_swapchain);

    link_parent_child(&m_camera_target, &m_camera.hierarchy);
    m_camera.hierarchy.local_transform.translation = {0.f, 0.f, m_target_distance};
    m_camera.hierarchy.local_transform.look_at(XMFLOAT3{0.0f, 0.0f, 0.0f}, XMFLOAT3{0.0f, 1.0f, 0.0f});
    mark_world_dirty(&m_camera_target);
}

struct ContextModel
{
    qhenki::gfx::Context* context;
    qhenki::gfx::CommandPool* pool;
    qhenki::gfx::QueueType queue;
    size_t model_count;
    GLTFModel* models;
    std::mutex* mutex;
    std::atomic_int* model_index_to_load_into;
    size_t cpu_buffer_srv_base;

    struct HeapAndList
    {
        qhenki::gfx::DescriptorHeap* heap;                 // Heap to use to create descriptors
        std::vector<qhenki::gfx::Descriptor>* descriptors; // (in/out) Descriptors to initialize
    };
    struct HeapAndDescriptor
    {
        qhenki::gfx::DescriptorHeap* heap;   // Heap to use to create descriptor
        qhenki::gfx::Descriptor* descriptor; // (in/out) Descriptor to initialize
    };
    HeapAndList texture;
    HeapAndList sampler;
    HeapAndDescriptor gltf_texture;
    HeapAndDescriptor material;
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
    if (!filelist || !*filelist)
    {
        return;
    }

    const auto context_model = static_cast<ContextModel*>(userdata);
    assert(context_model);

    ContextData context_data{
        .context = context_model->context,
        .pool = context_model->pool,
        .queue = context_model->queue,
    };

    auto& context = *context_model->context;

    std::scoped_lock lock(*context_model->mutex);

    if (load(*filelist, &context_model->models[*context_model->model_index_to_load_into], context_data))
    {
        printf("glTF model loaded: %s\n", *filelist);
    }
    else
    {
        printf("Failed to load glTF model: %s\n", *filelist);
    }

    // Only copy below descriptors when model lock is held so is ok.
    // Create views
    {
        auto& model = context_model->models[*context_model->model_index_to_load_into];

        auto& mat_heap = *context_model->material.heap;

        auto& tex_descriptors = *context_model->texture.descriptors;
        auto& tex_heap = *context_model->texture.heap;

        auto& samp_descriptors = *context_model->sampler.descriptors;
        auto& samp_heap = *context_model->sampler.heap;

        const size_t base = context_model->cpu_buffer_srv_base;

        assert(mat_heap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
        context_model->gltf_texture.descriptor->offset = base;
        context.create_descriptor_shader_view(model.texture_buffer, &mat_heap, context_model->gltf_texture.descriptor);

        // Material buffer Descriptor
        assert(mat_heap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
        context_model->material.descriptor->offset = base + 1;
        context.create_descriptor_shader_view(model.material_buffer, &mat_heap, context_model->material.descriptor);

        // Texture Descriptors
        if (model.images.size() > tex_descriptors.size())
        {
            tex_descriptors.resize(model.images.size());
        }
        for (size_t i = 0; i < model.images.size(); i++)
        {
            assert(tex_heap.desc.visibility == qhenki::gfx::DescriptorHeapDesc::Visibility::CPU);
            tex_descriptors[i].offset = base + 2 + i;
            context.create_descriptor_shader_view(model.images[i], &tex_heap, &tex_descriptors[i]);
        }

        // Sampler Descriptors
        if (model.samplers.size() > samp_descriptors.size())
        {
            samp_descriptors.resize(model.samplers.size());
        }
        for (size_t i = 0; i < model.samplers.size(); i++)
        {
            samp_descriptors[i].offset = i;
            context.create_descriptor(model.samplers[i], &samp_heap, &samp_descriptors[i]);
        }
    }
    *context_model->model_index_to_load_into = (*context_model->model_index_to_load_into + 1) %
                                               context_model->model_count; // Toggle model index
}

void gltfViewerApp::render()
{
    m_context->start_imgui_frame();

    const unsigned frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->acquire_swapchain_image());

    {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos;
        window_pos.x = work_pos.x + work_size.x - PAD;
        window_pos.y = work_pos.y + PAD;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, {1.0f, 0.0f});
        ImGui::SetNextWindowBgAlpha(0.5f);
        if (ImGui::Begin("Overlay",
                         nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
            // Display frame time graph
            constexpr size_t max_frames = 100;
            static float frame_times[max_frames];
            static size_t frame_index = 0;
            static bool buffer_filled = false;

            // Record new frame time
            frame_times[frame_index] = ImGui::GetIO().DeltaTime;
            frame_index = (frame_index + 1) % max_frames;
            if (frame_index == 0)
            {
                buffer_filled = true;
            }

            static float ordered_times[max_frames];
            size_t count = buffer_filled ? max_frames : frame_index;

            for (size_t i = 0; i < count; ++i)
            {
                size_t index = (frame_index + i) % max_frames;
                ordered_times[i] = frame_times[index];
            }

            ImGui::PlotLines(
                "##plot", ordered_times, max_frames, 0, "", 0.f, 0.05f, ImVec2(ImGui::GetContentRegionAvail().x, 40));
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Quit"))
            {
                m_QUIT = true;
            }
            if (ImGui::MenuItem("Load File"))
            {
                SDL_DialogFileFilter filter = {"glTF files (*.gltf;*glb)", "gltf;glb"};

                // The render thread will attempt to use model at m_model_index.
                // This should not cause issues unless model at m_model_index is being used in frame in flight,
                // which is impossible because you would have had to call this function twice in the same frame.

                thread_local ContextModel cm;
                cm = {
                    .context = m_context.get(),
                    .pool = &m_cmd_pools_thread[frame_slot], // Needs its own command pool for async loading
                    .queue = qhenki::gfx::QueueType::GRAPHICS,
                    .model_count = m_models.size(),
                    .models = m_models.data(),
                    .mutex = &m_model_mutex,
                    .model_index_to_load_into = &m_model_index_to_load_into,
                    .cpu_buffer_srv_base = m_frames_in_flight + (m_context->is_compatibility() ? 1u : 0u),
                    .texture{
                        .heap = &m_CPU_heap, // Use same CPU heap as matrix descriptors
                        .descriptors = &m_model_texture_descriptors,
                    },
                    .sampler{
                        .heap = &m_sampler_heap,
                        .descriptors = &m_sampler_descriptors,
                    },
                    .gltf_texture =
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
                SDL_ShowOpenFileDialog(callback, &cm, m_window.get_window(), &filter, 1, nullptr, false);
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::End();
    }

    const auto dim = this->m_window.get_display_size();

    m_camera.perspective.viewport_width = static_cast<float>(dim.x);
    m_camera.perspective.viewport_height = static_cast<float>(dim.y);

    update_world_transform(&m_camera.hierarchy);

    const bool left = m_input_manager.is_mouse_button_down(SDL_BUTTON_LEFT);
    if (ImGuiIO& io = ImGui::GetIO(); !io.WantCaptureMouse)
    {
        auto speed = 0.01f;
        const auto delta = m_input_manager.get_mouse_delta();

        const bool right = m_input_manager.is_mouse_button_down(SDL_BUTTON_RIGHT);
        SDL_SetWindowRelativeMouseMode(m_window.get_window(), left || right);
        if (left)
        {
            float y = delta.y * speed;
            const float x = delta.x * speed;
            const XMVECTOR yaw_delta = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), x);
            XMVECTOR rot = XMLoadFloat4(&m_camera_target.local_transform.rotation);
            rot = XMQuaternionMultiply(yaw_delta, rot); // Global
            const XMVECTOR right_vec = qhenki::math::axis_x(rot);
            const XMVECTOR pitch_delta = XMQuaternionRotationAxis(right_vec, y);
            rot = XMQuaternionMultiply(rot, pitch_delta); // Local
            XMStoreFloat4(&m_camera_target.local_transform.rotation, rot);
            mark_world_dirty(&m_camera_target);
        }

        if (right)
        {
            if (m_input_manager.is_key_down(SDL_SCANCODE_LSHIFT) || m_input_manager.is_key_down(SDL_SCANCODE_RSHIFT))
            {
                speed = 1.0f;
            }
            const XMVECTOR t = m_camera.hierarchy.world_transform.transform_vector(
                XMVectorSet(-delta.x * speed, delta.y * speed, 0.f, 0.f));
            m_camera_target.local_transform.translate_global(t);
            mark_world_dirty(&m_camera_target);
        }

        const auto middle = m_input_manager.is_mouse_button_down(SDL_BUTTON_MIDDLE);
        const auto scroll_y = m_input_manager.get_mouse_scroll().y;
        if (scroll_y != 0.0f || middle)
        {
            float amount = middle ? -delta.y : scroll_y * 0.2f;
            m_target_distance = std::max(0.01f, m_target_distance + amount);
            m_camera.hierarchy.local_transform.translation = XMFLOAT3(0.0f, 0.0f, m_target_distance);
            mark_world_dirty(&m_camera.hierarchy);
        }
    }
    update_world_transform(&m_camera.hierarchy);

    const XMMATRIX view =
        XMMatrixInverse(nullptr, qhenki::math::TransformSIMD::load(m_camera.hierarchy.world_transform).to_matrix());


    const XMMATRIX proj = XMMatrixPerspectiveFovLH(m_camera.perspective.fov,
                                                   m_camera.perspective.viewport_width /
                                                       m_camera.perspective.viewport_height,
                                                   m_camera.perspective.near_plane,
                                                   m_camera.perspective.far_plane);
    const XMMATRIX view_proj = XMMatrixMultiply(view, proj);

    CameraData camera_data;
    XMStoreFloat4x4(&camera_data.view_proj, XMMatrixTranspose(view_proj));
    XMStoreFloat4x4(&camera_data.inv_view_proj, XMMatrixTranspose(XMMatrixInverse(nullptr, view_proj)));
    camera_data.position = m_camera.hierarchy.world_transform.translation;

    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[frame_slot]);
    assert(buffer_pointer);
    memcpy(buffer_pointer, &camera_data, sizeof(CameraData));
    m_context->unmap_buffer(m_matrix_buffers[frame_slot]);

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[frame_slot]));

    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[frame_slot], m_cmd_pools[frame_slot]));
    auto& cmd_list = m_cmd_lists[frame_slot];

    // Resource transition
    qhenki::gfx::ImageBarrier barrier_render = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW, // Ensure we are not drawing anything to swapchain (still might
                                                        // be drawing from previous frame)
        .dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET, // Setting swapchain as render target requires
                                                                 // transition to finish first

        .src_access = qhenki::gfx::AccessFlags::ACCESS_COMMON,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,

        .src_layout = qhenki::gfx::Layout::PRESENT,
        .dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
    };
    m_context->set_barrier_resource(1, &barrier_render, m_swapchain);
    m_context->issue_barrier(&cmd_list, 1, &barrier_render);

    // Clear back buffer / Start render pass
    std::array clear_values = {0.f, 0.f, 0.f, 1.f};
    qhenki::gfx::RenderTarget depth{
        .clear_params = {.dsv_clear_params = {1.f, 0}},
        .clear_type = qhenki::gfx::RenderTarget::DEPTH,
        .texture = &m_depth_buffer,
    };
    m_context->start_render_pass(&cmd_list, clear_values.data(), &depth);

    // Set viewport
    const qhenki::gfx::Viewport viewport{
        .top_left_x = 0,
        .top_left_y = 0,
        .width = static_cast<float>(dim.x),
        .height = static_cast<float>(dim.y),
        .min_depth = 0.0f,
        .max_depth = 1.0f,
    };
    const qhenki::gfx::Rect scissor_rect{
        .left = 0,
        .top = 0,
        .width = dim.x,
        .height = dim.y,
    };
    m_context->set_viewports(&cmd_list, 1, &viewport);
    m_context->set_scissor_rects(&cmd_list, 1, &scissor_rect);

    m_context->set_descriptor_heap(&cmd_list, m_GPU_heap, m_sampler_heap);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_pipeline));

    // Bind resources
    if (m_context->is_compatibility())
    {
        m_context->compatibility_set_constant_buffers(0,
                                                      1,
                                                      qhenki::util::ptr_array(m_matrix_buffers[frame_slot]).data(),
                                                      qhenki::gfx::PipelineStage::VERTEX);
    }
    else
    {
        // Location of start of GPU heap
        qhenki::gfx::Descriptor descriptor(&m_GPU_heap, 0);

        // Parameter 1 is table, set to start at beginning of GPU heap
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 1, descriptor));

        // Copy matrix descriptors to GPU heap
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[frame_slot], descriptor));

        // Sampler
        descriptor = qhenki::gfx::Descriptor(&m_sampler_heap, 0);
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 2, descriptor));
    }

    { // Render
        // Draw glTF model
        std::unique_lock lock(m_model_mutex, std::defer_lock);
        const auto model_to_render = m_model_index_to_load_into > 0 ? m_model_index_to_load_into - 1
                                                                    : m_models.size() - 1;
        auto& m_model = m_models[model_to_render];
        if (lock.try_lock() && m_model.root_node >= 0) // If not still loading (because it is async function)
        {
            // Bindless bind textures, only need to do before all draws
            if (!m_context->is_compatibility()) // NOT compatibility
            {
                // Start after matrix CBV at heap offset 0
                qhenki::gfx::Descriptor descriptor(&m_GPU_heap, 1);

                // Make sure the order matches in the shader

                THROW_IF_FALSE(m_context->copy_descriptors(1, m_model_gltfTexture_descriptor, descriptor));
                ++descriptor.offset;

                THROW_IF_FALSE(m_context->copy_descriptors(1, m_model_material_descriptor, descriptor));
                ++descriptor.offset;

                // TODO: check that CPU texture descriptors are contiguous, otherwise singular copy will not work
                // Use the textures size, not descriptors because descriptors may be larger than texture count due to
                // left over from old model
                THROW_IF_FALSE(
                    m_context->copy_descriptors(m_model.textures.size(), m_model_texture_descriptors[0], descriptor));
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
                    auto m = qhenki::math::TransformSIMD::load(current_node.global_transform.transform).to_matrix();
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
                            assert(accessor.type == TINYGLTF_TYPE_SCALAR || accessor.type == TINYGLTF_TYPE_VEC2 ||
                                   accessor.type == TINYGLTF_TYPE_VEC3);

                            const auto& buffer_view = m_model.buffer_views[accessor.buffer_view];

                            auto buffer = &m_model.buffers[buffer_view.buffer_index];

                            assert(buffer_view.stride <= std::numeric_limits<unsigned>::max());
                            assert(accessor.offset <= std::numeric_limits<unsigned>::max());
                            auto stride = buffer_view.stride;
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

                            auto offset = accessor.offset + buffer_view.offset;

                            const uint64_t length = buffer_view.length;

                            m_context->bind_vertex_buffers(&cmd_list, slot, 1, &buffer, &length, &stride, &offset);
                        }
                    }
                    const auto& index_accessor = m_model.accessors[prim.indices];
                    const auto& buffer_view = m_model.buffer_views[index_accessor.buffer_view];

                    const auto& index_buffer = m_model.buffers[buffer_view.buffer_index];

                    auto index_type = index_accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                                        ? qhenki::gfx::IndexType::UINT16
                                        : qhenki::gfx::IndexType::UINT32;

                    unsigned index_offset = index_accessor.offset + buffer_view.offset;

                    m_context->bind_index_buffer(&cmd_list, index_buffer, index_type, index_offset);

                    if (m_context->is_compatibility())
                    {
                        ModelConstants model_push{};
                        memcpy(&model_push.model, &global_4x4, sizeof(XMFLOAT3X4));
                        memcpy(&model_push.inverse_model, &global_4x4_inverse, sizeof(XMFLOAT3X4));
                        model_push.material_index = prim.material_index;

                        const auto p = m_context->map_buffer(m_model_buffer);
                        memcpy(p, &model_push, sizeof(model_push));
                        m_context->unmap_buffer(m_model_buffer);

                        m_context->compatibility_set_constant_buffers(1,
                                                                      1,
                                                                      qhenki::util::ptr_array(m_model_buffer).data(),
                                                                      qhenki::gfx::PipelineStage::VERTEX);
                        m_context->compatibility_set_constant_buffers(1,
                                                                      1,
                                                                      qhenki::util::ptr_array(m_model_buffer).data(),
                                                                      qhenki::gfx::PipelineStage::PIXEL);

                        // Bind based off current material
                        const auto& material = m_model.materials[prim.material_index];
                        auto set_texture_if_valid = [&](const int slot, const int index)
                        {
                            if (index >= 0 && index < static_cast<int>(m_model_texture_descriptors.size()))
                            {
                                m_context->compatibility_set_textures(
                                    slot,
                                    1,
                                    qhenki::util::ptr_array(m_model_texture_descriptors[index]).data(),
                                    qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
                                    qhenki::gfx::PipelineStage::PIXEL);
                            }
                        };
                        // 5 textures
                        auto ret_t_index = [&](const int index)
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
                        auto sampler = [&](const int slot, const int index)
                        {
                            if (index < 0)
                            {
                                // Bind dummy sampler to silence validation warning
                                m_context->compatibility_set_samplers(slot,
                                                                      1,
                                                                      nullptr,
                                                                      qhenki::gfx::PipelineStage::PIXEL);
                                return;
                            }
                            const auto sampler_index = m_model.textures[index].sampler_index;
                            if (sampler_index < 0)
                            {
                                m_context->compatibility_set_samplers(slot,
                                                                      1,
                                                                      nullptr,
                                                                      qhenki::gfx::PipelineStage::PIXEL);
                            }
                            else
                            {
                                qhenki::gfx::Descriptor samp = m_sampler_descriptors[sampler_index];
                                m_context->compatibility_set_samplers(slot,
                                                                      1,
                                                                      qhenki::util::ptr_array(samp).data(),
                                                                      qhenki::gfx::PipelineStage::PIXEL);
                            }
                        };
                        start_slot = 0;
                        sampler(start_slot++, material.base_color.index);
                        sampler(start_slot++, material.metallic_roughness.index);
                        sampler(start_slot++, material.normal.index);
                        sampler(start_slot++, material.occlusion.index);
                        sampler(start_slot++, material.emissive.index);

                        // We will also bind the material itself here
                        m_context->compatibility_set_shader_buffers(
                            2,
                            1,
                            qhenki::util::ptr_array(m_model_material_descriptor).data(),
                            qhenki::gfx::PipelineStage::PIXEL);
                    }
                    else
                    {
                        ModelConstants model_push{};
                        memcpy(&model_push.model, &global_4x4, sizeof(XMFLOAT4X3));
                        memcpy(&model_push.inverse_model, &global_4x4_inverse, sizeof(XMFLOAT4X3));
                        model_push.material_index = prim.material_index;

                        m_context->set_pipeline_constant(
                            &cmd_list, m_pipeline_layout, 0, 0, sizeof(model_push), &model_push);
                    }

                    // Draw
                    m_context->draw_indexed(&cmd_list, index_accessor.count, 1, 0, 0, 0);
                }
            }
        }
    }

    ImGui::Render();
    m_context->render_imgui_draw_data(&cmd_list);

    m_context->end_render_pass(&cmd_list);

    // Resource transition
    qhenki::gfx::ImageBarrier barrier_present = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW, // Wait for all draws to swapchain to finish before
                                                        // transitioning to presentation
        .dst_stage = qhenki::gfx::SyncStage::SYNC_NONE, // No other stages will use swapchain resources

        .src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,

        .src_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .dst_layout = qhenki::gfx::Layout::PRESENT,
    };
    m_context->set_barrier_resource(1, &barrier_present, m_swapchain);
    m_context->issue_barrier(&cmd_list, 1, &barrier_present);

    // Close the command list
    m_context->close_command_list(&cmd_list);

    // Submit command list
    auto current_fence_value = m_fence_frame_ready_val[frame_slot];
    // To delete the model at m_model_index need to wait for this fence value
    // m_model_last_used_fence_value[m_model_index] = current_fence_value;
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
        .wait_swapchain = true,
        .signal_swapchain = true,
    };
    m_context->submit_command_lists(info, qhenki::gfx::QueueType::GRAPHICS);

    THROW_IF_FALSE(m_context->present(m_swapchain));

    const unsigned next_frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    auto next_fence_value = m_fence_frame_ready_val[next_frame_slot];
    if (m_context->get_fence_value(m_fence_frame_ready) < next_fence_value)
    {
        qhenki::gfx::WaitInfo wait_info{
            .wait_all = true,
            .count = 1,
            .fences = &m_fence_frame_ready,
            .values = &next_fence_value,
        };
        m_context->wait_fences(wait_info);
    }
    m_fence_frame_ready_val[next_frame_slot] = current_fence_value + 1;
}

void gltfViewerApp::resize(const unsigned width, const unsigned height)
{
    m_context->wait_idle(qhenki::gfx::QueueType::GRAPHICS);
    const qhenki::gfx::TextureDesc depth_desc{
        .width = width,
        .height = height,
        .format = qhenki::gfx::Format::D32_FLOAT,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
        .usage = qhenki::gfx::TextureDesc::DEPTH_STENCIL,
        .clear_depth_value = {1.f, 0},
    };
    THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, "Depth Buffer Texture"));
}

void gltfViewerApp::destroy()
{
    m_context->destroy_imgui();
}

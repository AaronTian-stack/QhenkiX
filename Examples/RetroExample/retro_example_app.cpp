#include "retro_example_app.h"
#include "shared_structs.h"

#include <imgui/imgui.h>

#include <qhenki/utility/file_util.h>
#include <qhenki/utility/general_util.h>
#include <qhenki/utility/math_util.h>
#include <qhenki/utility/string_util.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <memory>

#include "qhenki/math/transform_simd.h"

void RetroExampleApp::init_display_window(void* payload)
{
    qhenki::util::FormatResult<256> result;
    const char* title = "Retro Example";
    switch (get_graphics_api())
    {
    case qhenki::gfx::API::D3D11:
    {
        result = qhenki::util::format_string("%s | DX11", title);
        break;
    }
    case qhenki::gfx::API::D3D12:
    {
        result = qhenki::util::format_string("%s | DX12", title);
        break;
    }
    default:
    {
        result = qhenki::util::format_string("%s | undefined", title);
        break;
    }
    }

    bool fullscreen = false;
    if (payload)
    {
        const auto p = static_cast<Payload*>(payload);
        assert(p);
        fullscreen = p->fullscreen;
    }

    const qhenki::DisplayInfo info{
        .width = 1280,
        .height = 720,
        .fullscreen = fullscreen,
        .undecorated = false,
        .resizable = true,
        .title = result.buffer.data(),
    };

    m_window.create_window(info, 0);
}

void RetroExampleApp::create()
{
    const bool use_dx11 = m_context->is_compatibility();
    const char* subdir = use_dx11 ? "dx11" : "dx12";
    const char* vs_name = use_dx11 ? "BaseShader_vs_5_0_vs_main.dxbc" : "BaseShader_vs_6_6_vs_main.dxil";
    const char* ps_name = use_dx11 ? "BaseShader_ps_5_0_ps_main.dxbc" : "BaseShader_ps_6_6_ps_main.dxil";

    auto load_shader = [&](const char* name, const qhenki::gfx::ShaderType type, qhenki::gfx::Shader* out) -> bool
    {
        const auto path = qhenki::util::format_string("compiled-shaders/%s/%s", subdir, name);

        void* raw = nullptr;
        size_t size = 0;
        if (!qhenki::util::read_file(path.buffer.data(), &raw, &size))
        {
            return false;
        }
        const std::unique_ptr<std::byte, void (*)(void*)> data(static_cast<std::byte*>(raw), free);
        return m_context->create_shader(data.get(), size, type, out);
    };

    THROW_IF_FALSE(load_shader(vs_name, qhenki::gfx::VERTEX_SHADER, &m_vertex_shader));
    THROW_IF_FALSE(load_shader(ps_name, qhenki::gfx::PIXEL_SHADER, &m_pixel_shader));

    qhenki::gfx::LayoutBinding b1{
        .binding = 0,
        .count = 1,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    };
    qhenki::gfx::LayoutBinding b2{
        .binding = 1,
        .count = 1,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    };
    qhenki::gfx::LayoutBinding b3{
        .binding = 0,
        .count = 1,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
    };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {b1, b2};
    layout_desc.spaces[1] = {b3};
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .descriptor_count = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap, "GPU heap"));

    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .descriptor_count = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap, "CPU heap"));

    qhenki::gfx::DescriptorHeapDesc dsv_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::DSV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .descriptor_count = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(dsv_heap_desc, &m_dsv_heap, "DSV heap"));

    qhenki::gfx::DescriptorHeapDesc sampler_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .descriptor_count = 16,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap, "Sampler heap"));

    qhenki::gfx::GraphicsPipelineDesc pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .num_render_targets = 1,
        .dsv_format = DXGI_FORMAT_D32_FLOAT,
        .increment_slot = false,
    };
    THROW_IF_FALSE(m_context->create_pipeline(
        pipeline_desc, &m_pipeline, m_vertex_shader, m_pixel_shader, &m_pipeline_layout, "Retro pipeline"));

    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], m_graphics_queue));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    const auto display_size = m_window.get_display_size();
    qhenki::gfx::TextureDesc depth_desc{
        .width = display_size.x,
        .height = display_size.y,
        .format = DXGI_FORMAT_D32_FLOAT,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
    };
    THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, "Depth Buffer Texture"));
    THROW_IF_FALSE(m_context->create_descriptor_depth_stencil(m_depth_buffer, &m_dsv_heap, &m_depth_buffer_descriptor));

    qhenki::gfx::BufferDesc matrix_desc{.size = qhenki::util::align_u(sizeof(CameraMatrices),
                                                                      qhenki::util::CONSTANT_BUFFER_ALIGNMENT),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Matrix Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    qhenki::gfx::Buffer vertex_CPU;
    qhenki::gfx::Buffer index_CPU;

    constexpr auto vertices = std::array{Vertex{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f}},
                                         Vertex{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                         Vertex{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}};
    qhenki::gfx::BufferDesc desc{.size = vertices.size() * sizeof(Vertex),
                                 .usage = qhenki::gfx::BufferUsage::VERTEX,
                                 .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(m_context->create_buffer(desc, vertices.data(), &vertex_CPU, "Vertex Buffer CPU"));

    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_vertex_buffer, "Vertex Buffer GPU"));

    constexpr auto indices = std::array{0u, 1u, 2u};
    qhenki::gfx::BufferDesc index_desc{.size = indices.size() * sizeof(uint32_t),
                                       .usage = qhenki::gfx::BufferUsage::INDEX,
                                       .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(m_context->create_buffer(index_desc, indices.data(), &index_CPU, "Index Buffer CPU"));

    index_desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(index_desc, nullptr, &m_index_buffer, "Index Buffer GPU"));

    qhenki::gfx::TextureDesc texture_desc{
        .width = 4,
        .height = 4,
        .mip_levels = 3,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::COPY_DEST,
    };
    THROW_IF_FALSE(m_context->create_texture(texture_desc, &m_texture, "Checkerboard Texture"));

    THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_texture, &m_CPU_heap, &m_texture_descriptor));

    qhenki::gfx::SamplerDesc sampler_desc{
        .min_filter = qhenki::gfx::Filter::NEAREST,
        .mag_filter = qhenki::gfx::Filter::NEAREST,
    };
    THROW_IF_FALSE(m_context->create_descriptor(sampler_desc, &m_sampler_heap, &m_sampler_descriptor));

    constexpr auto checkerboard = std::array{
        0xFF0000FF, 0xFFFFFFFF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF0000FF, 0xFFFFFFFF,
        0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF0000FF,
        0xFFFFFFFF, 0xFF0000FF, 0xFFFF00FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF00FF, 0xFF00FFFF,
    };
    qhenki::gfx::Buffer texture_staging;

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list_init = m_cmd_lists[m_frame_index];
    m_context->copy_buffer(&cmd_list_init, vertex_CPU, 0, &m_vertex_buffer, 0, desc.size);
    m_context->copy_buffer(&cmd_list_init, index_CPU, 0, &m_index_buffer, 0, index_desc.size);

    THROW_IF_FALSE(m_context->copy_to_texture(&cmd_list_init, checkerboard.data(), &texture_staging, &m_texture));

    qhenki::gfx::ImageBarrier barrier_tex = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_NONE,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_NONE,
        .src_access = qhenki::gfx::AccessFlags::NO_ACCESS,
        .dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,
        .src_layout = qhenki::gfx::Layout::COPY_DEST,
        .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
    };
    m_context->set_barrier_resource(1, &barrier_tex, m_texture);
    m_context->issue_barrier(&cmd_list_init, 1, &barrier_tex);

    THROW_IF_FALSE(m_context->close_command_list(&cmd_list_init));
    auto current_fence_value = ++m_fence_frame_ready_val[m_frame_index];
    qhenki::gfx::SubmitInfo info_init{
        .command_list_count = 1,
        .command_lists = &cmd_list_init,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };
    m_context->submit_command_lists(info_init, &m_graphics_queue);

    qhenki::gfx::WaitInfo wait_info{.count = 1,
                                    .fences = &m_fence_frame_ready,
                                    .values = &m_fence_frame_ready_val[m_frame_index]};
    THROW_IF_FALSE(m_context->wait_fences(wait_info));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    m_context->init_imgui(m_window, m_swapchain);

    link_parent_child(&m_camera_target, &m_camera.hierarchy);
    m_camera.hierarchy.local_transform.translation = {0.f, 0.f, m_target_distance};
    m_camera.hierarchy.local_transform.look_at(XMFLOAT3{0.0f, 0.0f, 0.0f}, XMFLOAT3{0.0f, 1.0f, 0.0f});
    mark_world_dirty(&m_camera_target);
}

void RetroExampleApp::render()
{
    m_context->start_imgui_frame();
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
            constexpr size_t max_frames = 100;
            static float frame_times[max_frames];
            static size_t frame_index = 0;
            static bool buffer_filled = false;

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
            rot = XMQuaternionMultiply(yaw_delta, rot);
            const XMVECTOR right_vec = qhenki::math::axis_x(rot);
            const XMVECTOR pitch_delta = XMQuaternionRotationAxis(right_vec, y);
            rot = XMQuaternionMultiply(rot, pitch_delta);
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

    CameraMatrices matrices;
    XMStoreFloat4x4(&matrices.view_proj, XMMatrixTranspose(view_proj));
    XMStoreFloat4x4(&matrices.inv_view_proj, XMMatrixTranspose(XMMatrixInverse(nullptr, view_proj)));

    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[m_frame_index]);
    assert(buffer_pointer);
    memcpy(buffer_pointer, &matrices, sizeof(CameraMatrices));
    m_context->unmap_buffer(m_matrix_buffers[m_frame_index]);

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));

    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list = m_cmd_lists[m_frame_index];

    qhenki::gfx::ImageBarrier barrier_render = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_COMMON,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .src_layout = qhenki::gfx::Layout::PRESENT,
        .dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
    };
    m_context->set_barrier_resource(1, &barrier_render, m_swapchain, m_frame_index);
    m_context->issue_barrier(&cmd_list, 1, &barrier_render);

    std::array clear_values = {0.f, 0.f, 0.f, 1.f};
    qhenki::gfx::RenderTarget depth{
        .clear_params = {.dsv_clear_params = {1.f, 0}},
        .clear_type = qhenki::gfx::RenderTarget::DEPTH,
        .descriptor = m_depth_buffer_descriptor,
    };
    m_context->start_render_pass(&cmd_list, &m_swapchain, clear_values.data(), &depth, m_frame_index);

    const D3D12_VIEWPORT viewport{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<float>(dim.x),
        .Height = static_cast<float>(dim.y),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    const D3D12_RECT scissor_rect{
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

    if (m_context->is_compatibility())
    {
        m_context->compatibility_set_constant_buffers(0,
                                                      1,
                                                      qhenki::util::ptr_array(m_matrix_buffers[m_frame_index]).data(),
                                                      qhenki::gfx::PipelineStage::VERTEX);
        m_context->compatibility_set_textures(1,
                                              1,
                                              qhenki::util::ptr_array(m_texture_descriptor).data(),
                                              qhenki::gfx::ACCESS_SHADER_RESOURCE,
                                              qhenki::gfx::PipelineStage::PIXEL);
        m_context->compatibility_set_samplers(0,
                                              1,
                                              qhenki::util::ptr_array(m_sampler_descriptor).data(),
                                              qhenki::gfx::PipelineStage::PIXEL);
    }
    else
    {
        qhenki::gfx::Descriptor descriptor{.heap = &m_GPU_heap, .offset = 0};

        m_context->set_descriptor_table(&cmd_list, 0, descriptor);

        THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[m_frame_index], descriptor));
        descriptor.offset = 1;
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_texture_descriptor, descriptor));

        descriptor = {.heap = &m_sampler_heap, .offset = 0};
        m_context->set_descriptor_table(&cmd_list, 1, descriptor);
    }

    constexpr unsigned offset = 0;
    auto stride = static_cast<unsigned>(sizeof(Vertex));
    const auto buffers = &m_vertex_buffer;
    const auto unsigned_size = static_cast<unsigned>(sizeof(Vertex) * 3);
    m_context->bind_vertex_buffers(&cmd_list, 0, 1, &buffers, &unsigned_size, &stride, &offset);
    m_context->bind_index_buffer(&cmd_list, m_index_buffer, qhenki::gfx::IndexType::UINT32, 0);

    m_context->draw_indexed(&cmd_list, 3, 0, 0);

    ImGui::Render();
    m_context->render_imgui_draw_data(&cmd_list);

    qhenki::gfx::ImageBarrier barrier_present = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_NONE,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,
        .src_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .dst_layout = qhenki::gfx::Layout::PRESENT,
    };
    m_context->set_barrier_resource(1, &barrier_present, m_swapchain, m_frame_index);
    m_context->issue_barrier(&cmd_list, 1, &barrier_present);

    m_context->close_command_list(&cmd_list);

    auto current_fence_value = m_fence_frame_ready_val[m_frame_index];
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };
    m_context->submit_command_lists(info, &m_graphics_queue);

    m_context->present(&m_swapchain, 0, nullptr, m_frame_index);

    m_frame_index = m_context->get_swapchain_frame_index(m_swapchain);

    auto next_fence_value = m_fence_frame_ready_val[m_frame_index];
    if (m_context->get_fence_value(m_fence_frame_ready) < next_fence_value)
    {
        qhenki::gfx::WaitInfo wait_info{.wait_all = true,
                                        .count = 1,
                                        .fences = &m_fence_frame_ready,
                                        .values = &next_fence_value,
                                        .timeout = INFINITE};
        m_context->wait_fences(wait_info);
    }
    m_fence_frame_ready_val[m_frame_index] = current_fence_value + 1;
}

void RetroExampleApp::resize(int width, int height)
{
    m_context->wait_idle(&m_graphics_queue);
    const qhenki::gfx::TextureDesc depth_desc{
        .width = static_cast<uint64_t>(width),
        .height = static_cast<uint32_t>(height),
        .format = DXGI_FORMAT_D32_FLOAT,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
    };
    THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, "Depth Buffer Texture"));
    THROW_IF_FALSE(m_context->create_descriptor_depth_stencil(m_depth_buffer, &m_dsv_heap, &m_depth_buffer_descriptor));
}

void RetroExampleApp::destroy()
{
    m_context->destroy_imgui();
}

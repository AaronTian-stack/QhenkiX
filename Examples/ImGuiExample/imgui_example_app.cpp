#include "imgui_example_app.h"
#include "example_shared/shader_loader.h"
#include "example_shared/window_init.h"

#include <imgui/imgui.h>

#include <array>

void ImGUIExampleApp::init_display_window(void* payload)
{
    init_display_window_with_name(*this, m_window, "ImGUI Example", payload);
}

void ImGUIExampleApp::create()
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

    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    const auto bloated_descriptor_size = std::max(m_context->get_descriptor_size(qhenki::gfx::Descriptor::BUFFER),
                                                  m_context->get_descriptor_size(qhenki::gfx::Descriptor::TEXTURE));

    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .size = 256 * bloated_descriptor_size,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap));

    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .size = heap_desc_GPU.size,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap));

    qhenki::gfx::GraphicsPipelineDesc pipeline_desc = {
        .rtv_formats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .num_render_targets = 1,
        .increment_slot = false,
    };
    THROW_IF_FALSE(m_context->create_pipeline(
        pipeline_desc, &m_pipeline, vertex_shader, pixel_shader, &m_pipeline_layout, "Triangle pipeline"));


    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    qhenki::gfx::Buffer vertex_CPU;
    qhenki::gfx::Buffer index_CPU;

    constexpr auto vertices = std::array{Vertex{.position = {0.0f, 0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}},
                                         Vertex{.position = {0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}},
                                         Vertex{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    qhenki::gfx::BufferDesc desc{.size = vertices.size() * sizeof(Vertex),
                                 .usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::COPY_SRC,
                                 .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(
        m_context->create_buffer(desc, vertices.data(), &vertex_CPU, "Interleaved Position/Color Buffer CPU"));

    desc.usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::COPY_DST;
    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_vertex_buffer, "Interleaved Position/Color Buffer GPU"));

    constexpr auto indices = std::array{0u, 1u, 2u};
    qhenki::gfx::BufferDesc index_desc{.size = indices.size() * sizeof(uint32_t),
                                       .usage = qhenki::gfx::BufferUsage::INDEX | qhenki::gfx::BufferUsage::COPY_SRC,
                                       .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(m_context->create_buffer(index_desc, indices.data(), &index_CPU, "Index Buffer CPU"));

    index_desc.usage = qhenki::gfx::BufferUsage::INDEX | qhenki::gfx::BufferUsage::COPY_DST;
    index_desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(index_desc, nullptr, &m_index_buffer, "Index Buffer GPU"));

    // Schedule copies to GPU buffers / texture
    const unsigned frame_slot_init = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[frame_slot_init]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[frame_slot_init], m_cmd_pools[frame_slot_init]));
    auto& cmd_list = m_cmd_lists[frame_slot_init];
    m_context->copy_buffer(&cmd_list, vertex_CPU, 0, &m_vertex_buffer, 0, desc.size);
    m_context->copy_buffer(&cmd_list, index_CPU, 0, &m_index_buffer, 0, index_desc.size);

    THROW_IF_FALSE(m_context->close_command_list(&cmd_list));
    auto current_fence_value = ++m_fence_frame_ready_val[frame_slot_init];
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };

    m_context->submit_command_lists(info, qhenki::gfx::GRAPHICS);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Docking Branch
    m_context->init_imgui(m_window, m_swapchain);

    qhenki::gfx::WaitInfo wait_info{.count = 1,
                                    .fences = &m_fence_frame_ready,
                                    .values = &m_fence_frame_ready_val[frame_slot_init]};
    THROW_IF_FALSE(m_context->wait_fences(wait_info)); // Block CPU until done
}

void ImGUIExampleApp::render()
{
    m_context->start_imgui_frame();
    ImGui::ShowDemoWindow();

    const unsigned frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->acquire_swapchain_image());

    const auto dim = this->m_window.get_display_size();

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
    m_context->start_render_pass(&cmd_list, clear_values.data(), nullptr);

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
        .front = 0,
        .right = static_cast<long>(dim.x),
        .bottom = static_cast<long>(dim.y),
        .back = 0,
    };
    m_context->set_viewports(&cmd_list, 1, &viewport);
    m_context->set_scissor_rects(&cmd_list, 1, &scissor_rect);

    m_context->set_descriptor_heap(&cmd_list, m_GPU_heap);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_pipeline));

    constexpr uint64_t offset = 0;
    constexpr uint64_t stride = sizeof(Vertex);
    constexpr uint64_t size = 3 * sizeof(Vertex); // 3 vertices in triangle
    const auto buffers = &m_vertex_buffer;
    m_context->bind_vertex_buffers(&cmd_list, 0, 1, &buffers, &size, &stride, &offset);
    m_context->bind_index_buffer(&cmd_list, m_index_buffer, qhenki::gfx::IndexType::UINT32, 0);

    m_context->draw_indexed(&cmd_list, 3, 1, 0, 0, 0);

    // ImGUI Render
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
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
        .wait_swapchain = true,
        .signal_swapchain = true,
    };
    m_context->submit_command_lists(info, qhenki::gfx::GRAPHICS);

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

void ImGUIExampleApp::resize(unsigned width, unsigned height)
{
}

void ImGUIExampleApp::destroy()
{
    m_context->destroy_imgui();
}

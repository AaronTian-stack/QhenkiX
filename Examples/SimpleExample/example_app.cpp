#include "example_app.h"
#include <wrl/client.h>
#include "example_shared/macros.h"
#include "example_shared/shader_loader.h"
#include "example_shared/window_init.h"

#include "qhenki/utility/general_util.h"
#include "qhenki/utility/math_util.h"

#include <array>

void ExampleApp::init_display_window(void* payload)
{
    init_display_window_with_name(*this, m_window, "Simple Example", payload);
}

void ExampleApp::create()
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
    qhenki::gfx::LayoutBinding b1 // Constant buffer for camera matrix
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::CBV,
        };
    qhenki::gfx::LayoutBinding b2 // SRV for texture
        {
            .binding = 1, // TODO: figure out how to handle this for Vulkan
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::SRV,
        };
    qhenki::gfx::LayoutBinding b3 // Sampler for texture
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::SAMPLER,
        };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {b1, b2};
    layout_desc.spaces[1] = {b3}; // Samplers need their own space/table
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    // Create GPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .num_descriptors = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap));

    // Create CPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .num_descriptors = heap_desc_GPU.num_descriptors,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap));

    // Create Sampler Heap
    qhenki::gfx::DescriptorHeapDesc sampler_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU, // Create samplers directly on GPU heap
        .num_descriptors = 16,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap));

    // Create pipeline
    qhenki::gfx::GraphicsPipelineDesc pipeline_desc = {
        .rtv_formats = {qhenki::gfx::Format::R8G8B8A8_UNORM},
        .num_render_targets = 1,
        .increment_slot = false,
    };
    THROW_IF_FALSE(m_context->create_pipeline(
        pipeline_desc, &m_pipeline, vertex_shader, pixel_shader, &m_pipeline_layout, "triangle_pipeline"));

    // Allocate Command Pool(s)/Allocator(s) from queue
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    // Create vertex buffer
    constexpr auto vertices =
        std::array{Vertex{.position = {0.0f, 0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}, .texcoord = {0.5f, 1.0f}},
                   Vertex{.position = {0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}, .texcoord = {1.0f, 0.0f}},
                   Vertex{.position = {-0.5f, -0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 0.0f}}};
    qhenki::gfx::BufferDesc vertex_desc{.size = vertices.size() * sizeof(Vertex),
                                        .usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::COPY_DST,
                                        .visibility = qhenki::gfx::BufferVisibility::GPU};
    THROW_IF_FALSE(m_context->create_buffer(vertex_desc, nullptr, &m_vertex_buffer, "Vertex Buffer"));

    constexpr auto indices = std::array{0u, 1u, 2u};
    qhenki::gfx::BufferDesc index_desc{.size = indices.size() * sizeof(uint32_t),
                                       .usage = qhenki::gfx::BufferUsage::INDEX | qhenki::gfx::BufferUsage::COPY_DST,
                                       .visibility = qhenki::gfx::BufferVisibility::GPU};
    THROW_IF_FALSE(m_context->create_buffer(index_desc, nullptr, &m_index_buffer, "Index Buffer"));

    // Make 2 matrix constant buffers for double buffering
    qhenki::gfx::BufferDesc matrix_desc{.size = sizeof(CameraMatrices),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    // TODO: Persistent mapping flag
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        m_matrix_descriptors[i].offset = i;
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Matrix Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    // Create texture
    qhenki::gfx::TextureDesc texture_desc{
        .width = 4,
        .height = 4,
        .mip_levels = 3,
        .format = qhenki::gfx::Format::R8G8B8A8_UNORM,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::COPY_DEST,
        .usage = qhenki::gfx::TextureDesc::COPY_DEST | qhenki::gfx::TextureDesc::SHADER_RESOURCE,
    };
    THROW_IF_FALSE(m_context->create_texture(texture_desc, &m_texture, "Checkerboard Texture"));

    // Create CPU descriptor for texture
    m_texture_descriptor.offset = m_frames_in_flight;
    THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_texture, &m_CPU_heap, &m_texture_descriptor));

    // Create sampler
    qhenki::gfx::SamplerDesc sampler_desc{
        .min_filter = qhenki::gfx::Filter::NEAREST,
        .mag_filter = qhenki::gfx::Filter::NEAREST,
    }; // Default parameters
    THROW_IF_FALSE(m_context->create_descriptor(sampler_desc, &m_sampler_heap, &m_sampler_descriptor));

    // Texture data
    constexpr auto checkerboard = std::array{
        // Mip 0
        0xFF0000FF,
        0xFFFFFFFF,
        0xFF0000FF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFF0000FF,
        0xFFFFFFFF,
        0xFF0000FF,
        0xFF0000FF,
        0xFFFFFFFF,
        0xFF0000FF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFF0000FF,
        0xFFFFFFFF,
        0xFF0000FF,
        // Mip 1
        0xFFFF00FF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFF00FF,
        // Mip 2
        0xFF00FFFF,
    };

    const auto geometry_size = vertex_desc.size + index_desc.size;
    const auto texture_start = qhenki::util::align_up_non_power_of_two(geometry_size,
                                                                       m_context->get_staging_alignment(m_texture));
    const auto staging_size = texture_start + m_context->get_required_staging_size(m_texture);

    qhenki::gfx::BufferDesc staging_desc{
        .size = staging_size,
        .usage = qhenki::gfx::BufferUsage::COPY_SRC,
        .visibility = qhenki::gfx::CPU_SEQUENTIAL,
    };
    qhenki::gfx::Buffer staging; // Must keep in scope until copy is done
    THROW_IF_FALSE(m_context->create_buffer(staging_desc, nullptr, &staging, "Staging buffer"));

    {
        void* const ptr = m_context->map_buffer(staging);
        THROW_IF_FALSE(ptr);
        auto* const bytes = static_cast<std::byte*>(ptr);
        memcpy(bytes, vertices.data(), vertex_desc.size);
        memcpy(bytes + vertex_desc.size, indices.data(), index_desc.size);
        m_context->unmap_buffer(staging);
    }

    // Schedule copies to GPU buffers / texture
    const unsigned frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[frame_slot]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[frame_slot], m_cmd_pools[frame_slot]));
    auto& cmd_list = m_cmd_lists[frame_slot];
    m_context->copy_buffer(&cmd_list, staging, 0, &m_vertex_buffer, 0, vertex_desc.size);
    m_context->copy_buffer(&cmd_list, staging, vertex_desc.size, &m_index_buffer, 0, index_desc.size);

    qhenki::gfx::BufferRange range{
        .buffer = &staging,
        .offset = texture_start,
    };
    THROW_IF_FALSE(m_context->copy_to_texture(&cmd_list, checkerboard.data(), range, &m_texture));

    // Transition texture
    qhenki::gfx::ImageBarrier barrier_render = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_COPY,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,

        .src_access = qhenki::gfx::AccessFlags::ACCESS_COPY_DEST,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,

        .src_layout = qhenki::gfx::Layout::COPY_DEST,
        .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
    };
    m_context->set_barrier_resource(1, &barrier_render, m_texture);
    m_context->issue_barrier(&cmd_list, 1, &barrier_render);

    THROW_IF_FALSE(m_context->close_command_list(&cmd_list));
    auto wait_value = ++m_fence_frame_ready_val[frame_slot];
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &wait_value,
    };

    m_context->submit_command_lists(info, qhenki::gfx::GRAPHICS);

    qhenki::gfx::WaitInfo wait_info{.count = 1,
                                    .fences = &m_fence_frame_ready,
                                    .values = &m_fence_frame_ready_val[frame_slot]};
    THROW_IF_FALSE(m_context->wait_fences(wait_info)); // Block CPU until done
}

void ExampleApp::render()
{
    const unsigned frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->acquire_swapchain_image());

    const auto seconds_elapsed = static_cast<float>(SDL_GetTicks()) / 1000.f;

    // Update matrices
    XMVECTOR eye = XMVectorSet(0.0f, sinf(seconds_elapsed), -2.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(at, eye));
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
    XMVECTOR up = XMVector3Cross(right, forward);

    const auto view = XMMatrixLookAtLH(eye, at, up);
    const auto dim = this->m_window.get_display_size();
    const auto proj =
        XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(dim.x) / static_cast<float>(dim.y), 0.01f, 100.0f);
    const auto prod = XMMatrixTranspose(view * proj);
    XMStoreFloat4x4(&m_matrices.view_proj, prod);
    XMStoreFloat4x4(&m_matrices.inv_view_proj, XMMatrixInverse(nullptr, prod));

    // Update matrix buffer
    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[frame_slot]);
    THROW_IF_FALSE(buffer_pointer);
    memcpy(buffer_pointer, &m_matrices, sizeof(CameraMatrices));
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
        // Location of start of GPU heap
        qhenki::gfx::Descriptor descriptor{
            .heap = &m_GPU_heap,
            .offset = 0,
        };

        // Parameter 0 is table, set to start at beginning of GPU heap
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 0, descriptor));

        // Copy matrix and texture descriptors to GPU heap
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[frame_slot], descriptor));

        ++descriptor.offset;

        THROW_IF_FALSE(m_context->copy_descriptors(1, m_texture_descriptor, descriptor));

        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 1, m_sampler_descriptor));
    }

    constexpr uint64_t offset = 0;
    uint64_t stride = sizeof(Vertex);
    const auto buffers = &m_vertex_buffer;
    constexpr uint64_t unsigned_size = sizeof(Vertex) * 3;
    m_context->bind_vertex_buffers(&cmd_list, 0, 1, &buffers, &unsigned_size, &stride, &offset);
    m_context->bind_index_buffer(&cmd_list, m_index_buffer, qhenki::gfx::IndexType::UINT32, 0);

    m_context->draw_indexed(&cmd_list, 3, 1, 0, 0, 0);

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

    // If next frame is not ready to be used, wait until it is
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

void ExampleApp::resize(unsigned width, unsigned height)
{
}

void ExampleApp::destroy()
{
}

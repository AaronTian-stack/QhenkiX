#include "example_app.h"
#include <wrl/client.h>

#include <qhenki/utility/file_util.h>
#include "qhenki/utility/general_util.h"
#include "qhenki/utility/math_util.h"
#include "qhenki/utility/string_util.h"

#include <array>
#include <cstddef>
#include <memory>

void ExampleApp::create()
{
    const auto api = get_graphics_api();
    const bool use_dx11 = api == qhenki::gfx::API::D3D11;
    const bool use_vulkan = api == qhenki::gfx::API::Vulkan;
    const char* subdir = use_dx11 ? "dx11" : (use_vulkan ? "vulkan" : "dx12");

    const char* vs_name = nullptr;
    const char* ps_name = nullptr;
    if (use_dx11)
    {
        vs_name = "base_vs_5_0_vs_main.dxbc";
        ps_name = "base_ps_5_0_ps_main.dxbc";
    }
    else if (use_vulkan)
    {
        vs_name = "base_vs_6_6_vs_main.spv";
        ps_name = "base_ps_6_6_ps_main.spv";
    }
    else
    {
        vs_name = "base_vs_6_6_vs_main.dxil";
        ps_name = "base_ps_6_6_ps_main.dxil";
    }

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

    // Create pipeline layout
    qhenki::gfx::LayoutBinding b1 // Constant buffer for camera matrix
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::RangeType::CBV,
        };
    qhenki::gfx::LayoutBinding b2 // SRV for texture
        {
            .binding = 1, // TODO: figure out how to handle this for Vulkan
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::RangeType::SRV_TEXTURE,
        };
    qhenki::gfx::LayoutBinding b3 // Sampler for texture
        {
            .binding = 0,
            .count = 1,
            .type = qhenki::gfx::LayoutBinding::RangeType::SAMPLER,
        };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {b1, b2};
    layout_desc.spaces[1] = {b3}; // Samplers need their own space/table
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    const auto bloated_descriptor_size = std::max(m_context->get_descriptor_size(qhenki::gfx::Descriptor::BUFFER),
                                                  m_context->get_descriptor_size(qhenki::gfx::Descriptor::TEXTURE));

    // Create GPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .size = 256 * bloated_descriptor_size,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap));

    // Create CPU heap
    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .size = heap_desc_GPU.size,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap));

    // Create Sampler Heap
    qhenki::gfx::DescriptorHeapDesc sampler_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU, // Create samplers directly on GPU heap
        .size = 16 * m_context->get_descriptor_size(qhenki::gfx::Descriptor::SAMPLER),
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap));

    // Create pipeline
    qhenki::gfx::GraphicsPipelineDesc pipeline_desc = {
        .rtv_formats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .num_render_targets = 1,
        .increment_slot = false,
    };
    THROW_IF_FALSE(m_context->create_pipeline(
        pipeline_desc, &m_pipeline, m_vertex_shader, m_pixel_shader, &m_pipeline_layout, "triangle_pipeline"));

    // A graphics queue is already given to the application by the context

    // Allocate Command Pool(s)/Allocator(s) from queue
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    qhenki::gfx::Buffer vertex_CPU;
    qhenki::gfx::Buffer index_CPU;

    // Create vertex buffer
    constexpr auto vertices =
        std::array{Vertex{.position = {0.0f, 0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}, .texcoord = {0.5f, 1.0f}},
                   Vertex{.position = {0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}, .texcoord = {1.0f, 0.0f}},
                   Vertex{.position = {-0.5f, -0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 0.0f}}};
    qhenki::gfx::BufferDesc desc{.size = vertices.size() * sizeof(Vertex),
                                 .usage = qhenki::gfx::BufferUsage::VERTEX,
                                 .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(
        m_context->create_buffer(desc, vertices.data(), &vertex_CPU, "Interleaved Position/Color Buffer CPU"));

    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_vertex_buffer, "Interleaved Position/Color Buffer GPU"));

    constexpr auto indices = std::array{0u, 1u, 2u};
    qhenki::gfx::BufferDesc index_desc{.size = indices.size() * sizeof(uint32_t),
                                       .usage = qhenki::gfx::BufferUsage::INDEX,
                                       .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(m_context->create_buffer(index_desc, indices.data(), &index_CPU, "Index Buffer CPU"));

    index_desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(index_desc, nullptr, &m_index_buffer, "Index Buffer GPU"));

    // Make 2 matrix constant buffers for double buffering
    qhenki::gfx::BufferDesc matrix_desc{.size = sizeof(CameraMatrices),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    // TODO: Persistent mapping flag
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Matrix Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    // Create texture
    qhenki::gfx::TextureDesc texture_desc{
        .width = 4,
        .height = 4,
        .mip_levels = 3,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::COPY_DEST,
        .usage = qhenki::gfx::TextureDesc::COPY_DEST | qhenki::gfx::TextureDesc::SHADER_RESOURCE,
    };
    THROW_IF_FALSE(m_context->create_texture(texture_desc, &m_texture, "Checkerboard Texture"));

    // Create CPU descriptor for texture
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
    qhenki::gfx::Buffer texture_staging; // Must keep in scope until copy is done

    // Schedule copies to GPU buffers / texture
    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list = m_cmd_lists[m_frame_index];
    m_context->copy_buffer(&cmd_list, vertex_CPU, 0, &m_vertex_buffer, 0, desc.size);
    m_context->copy_buffer(&cmd_list, index_CPU, 0, &m_index_buffer, 0, index_desc.size);

    THROW_IF_FALSE(m_context->copy_to_texture(&cmd_list, checkerboard.data(), &texture_staging, &m_texture));

    // Transition texture
    qhenki::gfx::ImageBarrier barrier_render = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_NONE, // Not accessed before the barrier in same submission
        .dst_stage = qhenki::gfx::SyncStage::SYNC_NONE, // Not accessed after either

        .src_access = qhenki::gfx::AccessFlags::NO_ACCESS, // The resource is not accessed in this execution
        .dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,

        .src_layout = qhenki::gfx::Layout::COPY_DEST,
        .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
    };
    m_context->set_barrier_resource(1, &barrier_render, m_texture);
    m_context->issue_barrier(&cmd_list, 1, &barrier_render);

    THROW_IF_FALSE(m_context->close_command_list(&cmd_list));
    auto current_fence_value = ++m_fence_frame_ready_val[m_frame_index];
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };

    m_context->submit_command_lists(info, qhenki::gfx::GRAPHICS);

    qhenki::gfx::WaitInfo wait_info{.count = 1,
                                    .fences = &m_fence_frame_ready,
                                    .values = &m_fence_frame_ready_val[m_frame_index]};
    THROW_IF_FALSE(m_context->wait_fences(wait_info)); // Block CPU until done
}

void ExampleApp::render()
{
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
    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[m_frame_index]);
    THROW_IF_FALSE(buffer_pointer);
    memcpy(buffer_pointer, &m_matrices, sizeof(CameraMatrices));
    m_context->unmap_buffer(m_matrix_buffers[m_frame_index]);

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));

    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list = m_cmd_lists[m_frame_index];

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
    m_context->set_barrier_resource(1, &barrier_render, m_swapchain, m_frame_index);
    m_context->issue_barrier(&cmd_list, 1, &barrier_render);

    // Clear back buffer / Start render pass
    std::array clear_values = {0.f, 0.f, 0.f, 1.f};
    m_context->start_render_pass(&cmd_list, clear_values.data(), nullptr, m_frame_index);

    // Set viewport
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

    // Bind resources
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
        // Location of start of GPU heap
        qhenki::gfx::Descriptor descriptor(&m_GPU_heap, 0);

        // Parameter 0 is table, set to start at beginning of GPU heap
        m_context->set_descriptor_table(&cmd_list, 0, descriptor);

        // Copy matrix and texture descriptors to GPU heap
        THROW_IF_FALSE(m_context->copy_descriptors(m_context->get_descriptor_size(qhenki::gfx::Descriptor::BUFFER),
                                                   m_matrix_descriptors[m_frame_index],
                                                   descriptor));

        descriptor.offset =
            qhenki::util::align_u(descriptor.offset +
                                      1 * m_context->get_descriptor_size(qhenki::gfx::Descriptor::BUFFER),
                                  m_context->get_descriptor_alignment(qhenki::gfx::Descriptor::TEXTURE));

        THROW_IF_FALSE(m_context->copy_descriptors(m_context->get_descriptor_size(qhenki::gfx::Descriptor::TEXTURE),
                                                   m_texture_descriptor,
                                                   descriptor));

        // Sampler
        descriptor = qhenki::gfx::Descriptor(&m_sampler_heap, 0);
        m_context->set_descriptor_table(&cmd_list, 1, descriptor);
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
    m_context->set_barrier_resource(1, &barrier_present, m_swapchain, m_frame_index);
    m_context->issue_barrier(&cmd_list, 1, &barrier_present);

    // Close the command list
    m_context->close_command_list(&cmd_list);

    // Submit command list
    auto current_fence_value = m_fence_frame_ready_val[m_frame_index];
    qhenki::gfx::SubmitInfo info{
        .command_list_count = 1,
        .command_lists = &cmd_list,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };
    m_context->submit_command_lists(info, qhenki::gfx::GRAPHICS);

    // You MUST call Present at the end of the render loop
    // TODO: change for Vulkan
    THROW_IF_FALSE(m_context->present(m_swapchain, 0, nullptr, m_frame_index));

    m_frame_index = m_context->get_swapchain_frame_index(m_swapchain);

    // If next frame is not ready to be used, wait until it is
    auto next_fence_value = m_fence_frame_ready_val[m_frame_index];
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
    m_fence_frame_ready_val[m_frame_index] = current_fence_value + 1;
}

void ExampleApp::resize(unsigned width, unsigned height)
{
}

void ExampleApp::destroy()
{
}

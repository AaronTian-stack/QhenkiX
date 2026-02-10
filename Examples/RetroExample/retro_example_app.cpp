#include "retro_example_app.h"
#include "shared_structs.h"

#include <DirectXTex.h>

#include <imgui/imgui.h>

#include <qhenki/utility/file_util.h>
#include <qhenki/utility/general_util.h>
#include <qhenki/utility/math_util.h>
#include <qhenki/utility/shader_blob.h>
#include <qhenki/utility/string_util.h>

#include <array>
#include <cstddef>
#include <memory>

#include "qhenki/math/transform_simd.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef _DEBUG
#define TINYGLTF_NOEXCEPTION // Disable exception handling
#endif

#include <tiny_gltf.h>

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

    qhenki::gfx::LayoutBinding b1{
        .binding = 0,
        .count = 1,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    };
    qhenki::gfx::LayoutBinding b2{
        .binding = 1,
        .count = 2,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    };
    qhenki::gfx::LayoutBinding b3{
        .binding = 0,
        .count = 2,
        .type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
    };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {b1}; // CBV
    layout_desc.spaces[1] = {b2}; // SRV
    layout_desc.spaces[2] = {b3}; // Samplers
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

    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], m_graphics_queue));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    qhenki::gfx::BufferDesc matrix_desc{.size = qhenki::util::align_u(sizeof(FrameConstants),
                                                                      qhenki::util::CONSTANT_BUFFER_ALIGNMENT),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Frame Constant Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    qhenki::gfx::Buffer cylinder_CPU;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "assets/cylinder.glb");
    assert(model.meshes.size() == 1);
    assert(model.meshes[0].primitives.size() == 1);
    auto& prim = model.meshes[0].primitives[0];
    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
    }
    if (!ret)
    {
        printf("Failed to parse glTF\n");
    }

    auto set_accessor = [&model, this](Mesh::AccessorBufferView* abv, const int accessor_idx)
    {
        const auto& accessor = model.accessors[accessor_idx];
        abv->first = {
            .offset = accessor.byteOffset,
            .count = accessor.count,
            .type = accessor.type,
            .component_type = accessor.componentType,
        };
        const auto& buffer_view = model.bufferViews[accessor.bufferView];
        abv->second = {
            .offset = buffer_view.byteOffset,
            .length = buffer_view.byteLength,
            .stride = buffer_view.byteStride,
        };
    };

    set_accessor(&m_skybox_mesh.position, prim.attributes.at("POSITION"));
    set_accessor(&m_skybox_mesh.normal, prim.attributes.at("NORMAL"));
    set_accessor(&m_skybox_mesh.index, prim.indices);

    assert(model.buffers.size() == 1);
    auto& buffer = model.buffers[0];
    qhenki::gfx::BufferDesc desc{.size = buffer.data.size(),
                                 .usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX,
                                 .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &cylinder_CPU, "Skybox Vertex Buffer CPU"));

    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(desc, nullptr, &m_skybox_buffer, "Skybox Vertex Buffer GPU"));

    {
        void* ptr = m_context->map_buffer(cylinder_CPU);
        THROW_IF_FALSE(ptr);
        memcpy(ptr, buffer.data.data(), buffer.data.size());
        m_context->unmap_buffer(cylinder_CPU);
    }

    qhenki::gfx::SamplerDesc sampler_desc{
        .min_filter = qhenki::gfx::Filter::NEAREST,
        .mag_filter = qhenki::gfx::Filter::NEAREST,
        .mip_filter = qhenki::gfx::Filter::NEAREST,
        .address_mode_u = qhenki::gfx::AddressMode::WRAP,
        .address_mode_v = qhenki::gfx::AddressMode::WRAP,
        .address_mode_w = qhenki::gfx::AddressMode::WRAP,
    };
    THROW_IF_FALSE(m_context->create_descriptor(sampler_desc, &m_sampler_heap, &m_sampler_descriptor));

    qhenki::gfx::SamplerDesc sampler_linear_desc{
        .min_filter = qhenki::gfx::Filter::LINEAR,
        .mag_filter = qhenki::gfx::Filter::LINEAR,
        .mip_filter = qhenki::gfx::Filter::LINEAR,
        .address_mode_u = qhenki::gfx::AddressMode::CLAMP,
        .address_mode_v = qhenki::gfx::AddressMode::CLAMP,
        .address_mode_w = qhenki::gfx::AddressMode::CLAMP,
    };
    THROW_IF_FALSE(m_context->create_descriptor(sampler_linear_desc, &m_sampler_heap, &m_sampler_linear_descriptor));

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list_init = m_cmd_lists[m_frame_index];
    m_context->copy_buffer(&cmd_list_init, cylinder_CPU, 0, &m_skybox_buffer, 0, desc.size);

    tinygltf::Model cube_model;
    bool cube_ret = loader.LoadBinaryFromFile(&cube_model, &err, &warn, "assets/bevel_cube.glb");
    assert(cube_model.meshes.size() == 1);
    assert(cube_model.meshes[0].primitives.size() == 1);
    auto& cube_prim = cube_model.meshes[0].primitives[0];
    if (!cube_ret)
    {
        printf("Failed to parse bevel_cube.glb: %s\n", err.c_str());
    }
    auto set_accessor_cube = [&cube_model](Mesh::AccessorBufferView* abv, const int accessor_idx)
    {
        const auto& accessor = cube_model.accessors[accessor_idx];
        abv->first = {
            .offset = accessor.byteOffset,
            .count = accessor.count,
            .type = accessor.type,
            .component_type = accessor.componentType,
        };
        const auto& buffer_view = cube_model.bufferViews[accessor.bufferView];
        abv->second = {
            .offset = buffer_view.byteOffset,
            .length = buffer_view.byteLength,
            .stride = buffer_view.byteStride,
        };
    };
    set_accessor_cube(&m_bevel_cube_mesh.position, cube_prim.attributes.at("POSITION"));
    set_accessor_cube(&m_bevel_cube_mesh.normal, cube_prim.attributes.at("NORMAL"));
    set_accessor_cube(&m_bevel_cube_mesh.index, cube_prim.indices);

    assert(cube_model.buffers.size() == 1);
    auto& cube_buffer = cube_model.buffers[0];
    qhenki::gfx::BufferDesc cube_desc{
        .size = cube_buffer.data.size(),
        .usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX,
        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL,
    };
    qhenki::gfx::Buffer bevel_cube_CPU;
    THROW_IF_FALSE(m_context->create_buffer(cube_desc, nullptr, &bevel_cube_CPU, "Bevel Cube CPU"));
    {
        void* ptr = m_context->map_buffer(bevel_cube_CPU);
        THROW_IF_FALSE(ptr);
        memcpy(ptr, cube_buffer.data.data(), cube_buffer.data.size());
        m_context->unmap_buffer(bevel_cube_CPU);
    }
    cube_desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(m_context->create_buffer(cube_desc, nullptr, &m_bevel_cube_buffer, "Bevel Cube GPU"));
    m_context->copy_buffer(&cmd_list_init, bevel_cube_CPU, 0, &m_bevel_cube_buffer, 0, cube_buffer.data.size());

    qhenki::gfx::Buffer skybox_staging;

    const wchar_t* skybox_path = L"assets/skybox.dds";
    ScratchImage scratch;
    TexMetadata meta = {};
    const auto hr = LoadFromDDSFile(skybox_path, DDS_FLAGS_NONE, &meta, scratch);
    THROW_IF_TRUE(FAILED(hr));
    qhenki::gfx::TextureDesc skybox_tex_desc{
        .width = meta.width,
        .height = static_cast<uint32_t>(meta.height),
        .depth_or_array_size = static_cast<uint16_t>(meta.arraySize),
        .mip_levels = static_cast<uint16_t>(meta.mipLevels),
        .format = meta.format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::COPY_DEST,
    };
    THROW_IF_FALSE(m_context->create_texture(skybox_tex_desc, &m_skybox_texture, "Skybox Texture"));

    THROW_IF_FALSE(
        m_context->create_descriptor_shader_view(m_skybox_texture, &m_CPU_heap, &m_skybox_texture_descriptor));

    THROW_IF_FALSE(m_context->copy_to_texture(&cmd_list_init, scratch.GetPixels(), &skybox_staging, &m_skybox_texture));

    qhenki::gfx::ImageBarrier barrier_skybox = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_COPY,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_COPY_DEST,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
        .src_layout = qhenki::gfx::Layout::COPY_DEST,
        .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
    };
    m_context->set_barrier_resource(1, &barrier_skybox, m_skybox_texture);
    m_context->issue_barrier(&cmd_list_init, 1, &barrier_skybox);

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

    const char* skybox_vs_name = use_dx11 ? "skybox_vs_5_0_vs_main.dxbc" : "skybox_vs_6_6_vs_main.dxil";
    const char* skybox_ps_name = use_dx11 ? "skybox_ps_5_0_ps_main.dxbc" : "skybox_ps_6_6_ps_main.dxil";
    THROW_IF_FALSE(load_shader(skybox_vs_name, qhenki::gfx::VERTEX_SHADER, &m_skybox_vertex_shader));
    THROW_IF_FALSE(load_shader(skybox_ps_name, qhenki::gfx::PIXEL_SHADER, &m_skybox_pixel_shader));

    D3D12_BLEND_DESC skybox_blend_desc{
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget =
            {
                {
                    .BlendEnable = TRUE,
                    .LogicOpEnable = FALSE,
                    .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                    .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                    .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_ONE,
                    .DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
                    .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .LogicOp = D3D12_LOGIC_OP_NOOP,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                },
            },
    };
    qhenki::gfx::DepthStencilDesc skybox_depth_desc{};
    skybox_depth_desc.depth_func = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    qhenki::gfx::GraphicsPipelineDesc skybox_pipeline_desc = {
        .blend_desc = skybox_blend_desc,
        .depth_stencil_state = skybox_depth_desc,
        .rtv_formats = {m_offscreen_rt_format},
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = false,
    };
    THROW_IF_FALSE(m_context->create_pipeline(skybox_pipeline_desc,
                                              &m_skybox_pipeline,
                                              m_skybox_vertex_shader,
                                              m_skybox_pixel_shader,
                                              &m_pipeline_layout, // Reuse existing layout (CBV + SRV + sampler)
                                              "Skybox pipeline"));

    const char* cube_vs_name = use_dx11 ? "cube_vs_5_0_vs_main.dxbc" : "cube_vs_6_6_vs_main.dxil";
    const char* cube_ps_name = use_dx11 ? "cube_ps_5_0_ps_main.dxbc" : "cube_ps_6_6_ps_main.dxil";
    THROW_IF_FALSE(load_shader(cube_vs_name, qhenki::gfx::VERTEX_SHADER, &m_cube_vertex_shader));
    THROW_IF_FALSE(load_shader(cube_ps_name, qhenki::gfx::PIXEL_SHADER, &m_cube_pixel_shader));

    qhenki::gfx::GraphicsPipelineDesc cube_pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {m_offscreen_rt_format},
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(cube_pipeline_desc,
                                              &m_cube_pipeline,
                                              m_cube_vertex_shader,
                                              m_cube_pixel_shader,
                                              &m_pipeline_layout,
                                              "Cube pipeline"));

    const char* bevel_vs_name = use_dx11 ? "cube_instanced_vs_5_0_vs_main.dxbc" : "cube_instanced_vs_6_6_vs_main.dxil";
    const char* bevel_ps_name = use_dx11 ? "cube_instanced_ps_5_0_ps_main.dxbc" : "cube_instanced_ps_6_6_ps_main.dxil";
    THROW_IF_FALSE(load_shader(bevel_vs_name, qhenki::gfx::VERTEX_SHADER, &m_bevel_cube_vertex_shader));
    THROW_IF_FALSE(load_shader(bevel_ps_name, qhenki::gfx::PIXEL_SHADER, &m_bevel_cube_pixel_shader));
    qhenki::gfx::GraphicsPipelineDesc bevel_pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {m_offscreen_rt_format},
        .rasterizer_state =
            qhenki::gfx::RasterizerDesc{
                .cull_mode = D3D12_CULL_MODE_BACK,
                .front_counter_clockwise = false,
            },
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(bevel_pipeline_desc,
                                              &m_bevel_cube_pipeline,
                                              m_bevel_cube_vertex_shader,
                                              m_bevel_cube_pixel_shader,
                                              &m_pipeline_layout,
                                              "Bevel cube instanced pipeline"));

    const char* blit_vs_name = use_dx11 ? "fullscreen_triangle_vs_5_0_vs_main.dxbc"
                                        : "fullscreen_triangle_vs_6_6_vs_main.dxil";
    const char* blit_copy_ps_name = use_dx11 ? "blit_copy_ps_5_0_ps_main.dxbc" : "blit_copy_ps_6_6_ps_main.dxil";
    const char* blit_lum_ps_name = use_dx11 ? "blit_luminance_ps_5_0_ps_main.dxbc"
                                            : "blit_luminance_ps_6_6_ps_main.dxil";
    const char* blit_bloom_blob_name = use_dx11 ? "blit_bloom_1d_ps_5_0_ps_main.dxbc_blob"
                                                : "blit_bloom_1d_ps_6_6_ps_main.dxil_blob";
    THROW_IF_FALSE(load_shader(blit_vs_name, qhenki::gfx::VERTEX_SHADER, &m_blit_vertex_shader));
    THROW_IF_FALSE(load_shader(blit_copy_ps_name, qhenki::gfx::PIXEL_SHADER, &m_blit_copy_pixel_shader));
    THROW_IF_FALSE(load_shader(blit_lum_ps_name, qhenki::gfx::PIXEL_SHADER, &m_blit_luminance_pixel_shader));
    {
        const auto path = qhenki::util::format_string("compiled-shaders/%s/%s", subdir, blit_bloom_blob_name);
        void* raw = nullptr;
        size_t blob_size = 0;
        THROW_IF_FALSE(qhenki::util::read_file(path.buffer.data(), &raw, &blob_size));
        const std::unique_ptr<std::byte, void (*)(void*)> blob_data(static_cast<std::byte*>(raw), free);
        const char* horizontal_defines[] = {"BLUR_HORIZONTAL=1"};
        const char* vertical_defines[] = {"BLUR_HORIZONTAL=0"};
        void* shader_ptr = nullptr;
        size_t shader_size = 0;
        THROW_IF_FALSE(qhenki::util::find_permutation_in_blob(
            blob_data.get(), blob_size, horizontal_defines, 1, &shader_ptr, &shader_size));
        THROW_IF_FALSE(m_context->create_shader(
            shader_ptr, shader_size, qhenki::gfx::PIXEL_SHADER, &m_blit_bloom_1d_horizontal_pixel_shader));
        THROW_IF_FALSE(qhenki::util::find_permutation_in_blob(
            blob_data.get(), blob_size, vertical_defines, 1, &shader_ptr, &shader_size));
        THROW_IF_FALSE(m_context->create_shader(
            shader_ptr, shader_size, qhenki::gfx::PIXEL_SHADER, &m_blit_bloom_1d_vertical_pixel_shader));
    }

    qhenki::gfx::GraphicsPipelineDesc blit_pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {m_offscreen_rt_format},
        .num_render_targets = 1,
        .dsv_format = DXGI_FORMAT_UNKNOWN,
        .increment_slot = true,
    };
    qhenki::gfx::GraphicsPipelineDesc blit_copy_pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {m_swapchain.desc.format},
        .num_render_targets = 1,
        .dsv_format = DXGI_FORMAT_UNKNOWN,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(blit_copy_pipeline_desc,
                                              &m_blit_copy_pipeline,
                                              m_blit_vertex_shader,
                                              m_blit_copy_pixel_shader,
                                              &m_pipeline_layout,
                                              "Blit copy pipeline"));
    THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                              &m_blit_luminance_pipeline,
                                              m_blit_vertex_shader,
                                              m_blit_luminance_pixel_shader,
                                              &m_pipeline_layout,
                                              "Blit luminance pipeline"));
    THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                              &m_blit_bloom_1d_horizontal_pipeline,
                                              m_blit_vertex_shader,
                                              m_blit_bloom_1d_horizontal_pixel_shader,
                                              &m_pipeline_layout,
                                              "Blit bloom 1D horizontal pipeline"));
    THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                              &m_blit_bloom_1d_vertical_pipeline,
                                              m_blit_vertex_shader,
                                              m_blit_bloom_1d_vertical_pixel_shader,
                                              &m_pipeline_layout,
                                              "Blit bloom 1D vertical pipeline"));

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

    link_parent_child(&m_cube_parent, &m_cube_child);
    link_parent_child(&m_cube_parent, &m_cube_camera.hierarchy);
    m_cube_camera.perspective.fov = m_camera.perspective.fov;
    m_cube_camera.perspective.near_plane = m_camera.perspective.near_plane;
    m_cube_camera.perspective.far_plane = m_camera.perspective.far_plane;

    m_orbit_camera.perspective.fov = m_camera.perspective.fov;
    m_orbit_camera.perspective.near_plane = m_camera.perspective.near_plane;
    m_orbit_camera.perspective.far_plane = m_camera.perspective.far_plane;

    mark_world_dirty(&m_cube_parent);
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

            ImGui::End();
        }
    }

    const auto dim = this->m_window.get_display_size();

    m_camera.perspective.viewport_width = static_cast<float>(dim.x);
    m_camera.perspective.viewport_height = static_cast<float>(dim.y);
    m_cube_camera.perspective.viewport_width = static_cast<float>(dim.x);
    m_cube_camera.perspective.viewport_height = static_cast<float>(dim.y);
    m_orbit_camera.perspective.viewport_width = static_cast<float>(dim.x);
    m_orbit_camera.perspective.viewport_height = static_cast<float>(dim.y);

    update_world_transform(&m_camera.hierarchy);

    const bool left = m_input_manager.is_mouse_button_down(SDL_BUTTON_LEFT);
    if (m_active_camera_index == 0 && !ImGui::GetIO().WantCaptureMouse)
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

    const float time_sec = static_cast<float>(SDL_GetTicks()) / 1000.f;

    constexpr float radius_min = 6.f;
    constexpr float radius_max = 10.f;
    const float radius_t = 0.5f + 0.5f * std::sin(time_sec * 0.7f);
    const float orbit_radius = radius_min + (radius_max - radius_min) * radius_t;
    const float orbit_angle = time_sec * 0.5f;
    m_cube_parent.local_transform.translation.x = orbit_radius * std::cos(orbit_angle);
    m_cube_parent.local_transform.translation.y = (1.2f + std::sin(time_sec * 2.f)) * 0.5f * 8.f;
    m_cube_parent.local_transform.translation.z = orbit_radius * std::sin(orbit_angle);

    const float axis_angle = time_sec * 0.4f;
    const float rot_angle = time_sec * 1.2f;
    XMVECTOR rot_axis = XMVectorSet(std::sin(axis_angle), 0.6f, std::cos(axis_angle), 0.f);
    rot_axis = XMVector3Normalize(rot_axis);
    XMStoreFloat4(&m_cube_child.local_transform.rotation, XMQuaternionRotationAxis(rot_axis, rot_angle));

    m_cube_child.local_transform.scale = {0.8f, 0.8f, 0.8f};

    const float cam_y_angle = time_sec * 0.2f;
    m_cube_camera.hierarchy.local_transform.translation.x = 6.f * std::sin(cam_y_angle);
    m_cube_camera.hierarchy.local_transform.translation.y = 2.f;
    m_cube_camera.hierarchy.local_transform.translation.z = 6.f * std::cos(cam_y_angle);
    m_cube_camera.hierarchy.local_transform.scale = qhenki::math::Transform::identity_scale();
    m_cube_camera.hierarchy.local_transform.look_at(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f));

    constexpr float orbit_distance = 24.0f;
    const float orbit_cam_angle = -time_sec * 0.4f;
    m_orbit_camera.hierarchy.local_transform.translation.x = orbit_distance * std::cos(orbit_cam_angle);
    m_orbit_camera.hierarchy.local_transform.translation.y = 8.0f;
    m_orbit_camera.hierarchy.local_transform.translation.z = orbit_distance * std::sin(orbit_cam_angle);
    m_orbit_camera.hierarchy.local_transform.look_at(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f));
    mark_world_dirty(&m_orbit_camera.hierarchy);
    update_world_transform(&m_orbit_camera.hierarchy);

    mark_world_dirty(&m_cube_parent);
    update_world_transform(&m_cube_parent);

    static float m_camera_transition_t = -1.f;
    static qhenki::math::Transform m_camera_transition_start;
    {
        static bool r_was_down = false;
        const bool r_down = m_input_manager.is_key_down(SDL_SCANCODE_R);
        const bool arcball_grabbing = (m_active_camera_index == 0 &&
                                       (m_input_manager.is_mouse_button_down(SDL_BUTTON_LEFT) ||
                                        m_input_manager.is_mouse_button_down(SDL_BUTTON_RIGHT)));
        if (r_down && !r_was_down && !arcball_grabbing)
        {
            PerspectiveCamera& current = m_active_camera_index == 0
                                           ? m_camera
                                           : (m_active_camera_index == 1 ? m_cube_camera : m_orbit_camera);
            if (m_camera_transition_t < 0.f)
            {
                m_camera_transition_start = current.hierarchy.world_transform;
            }
            else
            {
                const float blend = std::min(m_camera_transition_t, 1.f);
                const auto& from = m_camera_transition_start;
                const auto& to = current.hierarchy.world_transform;
                XMStoreFloat3(&m_camera_transition_start.translation,
                              XMVectorLerp(XMLoadFloat3(&from.translation), XMLoadFloat3(&to.translation), blend));
                XMStoreFloat4(&m_camera_transition_start.rotation,
                              XMQuaternionSlerp(XMLoadFloat4(&from.rotation), XMLoadFloat4(&to.rotation), blend));
                XMStoreFloat3(&m_camera_transition_start.scale,
                              XMVectorLerp(XMLoadFloat3(&from.scale), XMLoadFloat3(&to.scale), blend));
            }
            m_active_camera_index = (m_active_camera_index + 1) % 3;
            m_camera_transition_t = 0.f;
        }
        r_was_down = r_down;
    }

    if (m_camera_transition_t >= 0.f)
    {
        m_camera_transition_t += ImGui::GetIO().DeltaTime * 2.f;
        if (m_camera_transition_t >= 1.f)
        {
            m_camera_transition_t = -1.f;
        }
    }

    const XMMATRIX cube_world_mat = qhenki::math::TransformSIMD::load(m_cube_child.world_transform).to_matrix();

    const PerspectiveCamera& active_camera = m_active_camera_index == 0 ? m_camera
                                           : m_active_camera_index == 1 ? m_cube_camera
                                                                        : m_orbit_camera;

    qhenki::math::Transform display_world;
    if (m_camera_transition_t >= 0.f)
    {
        const float blend = std::min(m_camera_transition_t, 1.f);
        const auto& from = m_camera_transition_start;
        const auto& to = active_camera.hierarchy.world_transform;
        XMStoreFloat3(&display_world.translation,
                      XMVectorLerp(XMLoadFloat3(&from.translation), XMLoadFloat3(&to.translation), blend));
        XMStoreFloat4(&display_world.rotation,
                      XMQuaternionSlerp(XMLoadFloat4(&from.rotation), XMLoadFloat4(&to.rotation), blend));
        XMStoreFloat3(&display_world.scale, XMVectorLerp(XMLoadFloat3(&from.scale), XMLoadFloat3(&to.scale), blend));
    }
    else
    {
        display_world = active_camera.hierarchy.world_transform;
    }

    const XMMATRIX view = XMMatrixInverse(nullptr, qhenki::math::TransformSIMD::load(display_world).to_matrix());
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(active_camera.perspective.fov,
                                                   active_camera.perspective.viewport_width /
                                                       active_camera.perspective.viewport_height,
                                                   active_camera.perspective.near_plane,
                                                   active_camera.perspective.far_plane);
    const XMMATRIX view_proj = XMMatrixMultiply(view, proj);

    FrameConstants cb;
    XMStoreFloat4x4(&cb.camera_buffer.view_proj, XMMatrixTranspose(view_proj));
    XMStoreFloat4x4(&cb.camera_buffer.inv_view_proj, XMMatrixTranspose(XMMatrixInverse(nullptr, view_proj)));
    XMStoreFloat4x4(&cb.cube_world, XMMatrixTranspose(cube_world_mat));
    cb.camera_position = display_world.translation;
    cb.time = time_sec;

    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[m_frame_index]);
    assert(buffer_pointer);
    memcpy(buffer_pointer, &cb, sizeof(FrameConstants));
    m_context->unmap_buffer(m_matrix_buffers[m_frame_index]);

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[m_frame_index]));

    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[m_frame_index], m_cmd_pools[m_frame_index]));
    auto& cmd_list = m_cmd_lists[m_frame_index];

    qhenki::gfx::RenderTarget color{
        .clear_params = {.clear_color_value = {1.f, 1.f, 1.f, 0.f}},
        .clear_type = qhenki::gfx::RenderTarget::COLOR,
        .descriptor = m_offscreen_texture.rt_descriptor,
    };
    qhenki::gfx::RenderTarget depth{
        .clear_params = {.dsv_clear_params = {1.f, 0}},
        .clear_type = qhenki::gfx::RenderTarget::DEPTH,
        .descriptor = m_depth_buffer_descriptor,
    };
    m_context->start_render_pass(&cmd_list, 1, qhenki::util::ptr_array(color).data(), &depth);

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

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_skybox_pipeline));
    size_t gpu_descriptor_heap_index = 0;
    if (m_context->is_compatibility())
    {
        std::array buffer = {&m_matrix_buffers[m_frame_index]};
        m_context->compatibility_set_constant_buffers(0,
                                                      buffer.size(),
                                                      buffer.data(),
                                                      qhenki::gfx::PipelineStage::VERTEX);
        m_context->compatibility_set_constant_buffers(0,
                                                      buffer.size(),
                                                      buffer.data(),
                                                      qhenki::gfx::PipelineStage::PIXEL);
        m_context->compatibility_set_textures(1,
                                              1,
                                              qhenki::util::ptr_array(m_skybox_texture_descriptor).data(),
                                              qhenki::gfx::ACCESS_SHADER_RESOURCE,
                                              qhenki::gfx::PipelineStage::PIXEL);
        m_context->compatibility_set_samplers(
            0,
            2,
            qhenki::util::ptr_array(m_sampler_descriptor, m_sampler_linear_descriptor).data(),
            qhenki::gfx::PipelineStage::PIXEL);
    }
    else
    {
        qhenki::gfx::Descriptor cbv_descriptor{.heap = &m_GPU_heap, .offset = gpu_descriptor_heap_index++};
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[m_frame_index], cbv_descriptor));
        m_context->set_descriptor_table(&cmd_list, 0, cbv_descriptor);

        qhenki::gfx::Descriptor srv_descriptor{.heap = &m_GPU_heap, .offset = gpu_descriptor_heap_index++};
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_skybox_texture_descriptor, srv_descriptor));
        m_context->set_descriptor_table(&cmd_list, 1, srv_descriptor);

        qhenki::gfx::Descriptor sampler_descriptor{.heap = &m_sampler_heap, .offset = 0};
        m_context->set_descriptor_table(&cmd_list, 2, sampler_descriptor);
    }
    auto stride_from_accessor = [](const int component_type, const int type)
    {
        const unsigned comp_size = component_type == TINYGLTF_PARAMETER_TYPE_FLOAT            ? 4u
                                 : (component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) ? 2u
                                 : (component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)   ? 4u
                                                                                              : 4u;
        const unsigned comp_count = type == TINYGLTF_TYPE_SCALAR ? 1u : type == TINYGLTF_TYPE_VEC2 ? 2u : 3u;
        return comp_size * comp_count;
    };
    auto vb_offset = [](const Mesh::AccessorBufferView& abv) -> unsigned
    {
        return static_cast<unsigned>(abv.second.offset + abv.first.offset);
    };
    auto vb_length = [](const Mesh::AccessorBufferView& abv) -> unsigned
    {
        return static_cast<unsigned>(abv.second.length);
    };
    auto vb_stride = [&stride_from_accessor](const Mesh::AccessorBufferView& abv) -> unsigned
    {
        return abv.second.stride != 0 ? static_cast<unsigned>(abv.second.stride)
                                      : stride_from_accessor(abv.first.component_type, abv.first.type);
    };
    const std::array skybox_vbs = {&m_skybox_buffer, &m_skybox_buffer};
    const std::array skybox_vb_offsets = {vb_offset(m_skybox_mesh.position), vb_offset(m_skybox_mesh.normal)};
    const std::array skybox_vb_lengths = {vb_length(m_skybox_mesh.position), vb_length(m_skybox_mesh.normal)};
    const std::array skybox_vb_strides = {vb_stride(m_skybox_mesh.position), vb_stride(m_skybox_mesh.normal)};
    m_context->bind_vertex_buffers(&cmd_list,
                                   0,
                                   2,
                                   skybox_vbs.data(),
                                   skybox_vb_lengths.data(),
                                   skybox_vb_strides.data(),
                                   skybox_vb_offsets.data());
    const unsigned index_offset = static_cast<unsigned>(m_skybox_mesh.index.second.offset +
                                                        m_skybox_mesh.index.first.offset);
    const auto index_type = m_skybox_mesh.index.first.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                              ? qhenki::gfx::IndexType::UINT16
                              : qhenki::gfx::IndexType::UINT32;
    m_context->bind_index_buffer(&cmd_list, m_skybox_buffer, index_type, index_offset);
    m_context->draw_indexed(&cmd_list, static_cast<unsigned>(m_skybox_mesh.index.first.count), 1, 0, 0, 0);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_cube_pipeline));
    m_context->draw(&cmd_list, 36u, 0);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_bevel_cube_pipeline));
    const std::array bevel_vbs = {&m_bevel_cube_buffer, &m_bevel_cube_buffer};
    const std::array bevel_vb_offsets = {vb_offset(m_bevel_cube_mesh.position), vb_offset(m_bevel_cube_mesh.normal)};
    const std::array bevel_vb_lengths = {vb_length(m_bevel_cube_mesh.position), vb_length(m_bevel_cube_mesh.normal)};
    const std::array bevel_vb_strides = {vb_stride(m_bevel_cube_mesh.position), vb_stride(m_bevel_cube_mesh.normal)};
    m_context->bind_vertex_buffers(
        &cmd_list, 0, 2, bevel_vbs.data(), bevel_vb_lengths.data(), bevel_vb_strides.data(), bevel_vb_offsets.data());
    const unsigned bevel_index_offset = static_cast<unsigned>(m_bevel_cube_mesh.index.second.offset +
                                                              m_bevel_cube_mesh.index.first.offset);
    const auto bevel_index_type = m_bevel_cube_mesh.index.first.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                                    ? qhenki::gfx::IndexType::UINT16
                                    : qhenki::gfx::IndexType::UINT32;
    m_context->bind_index_buffer(&cmd_list, m_bevel_cube_buffer, bevel_index_type, bevel_index_offset);
    constexpr unsigned bevel_instance_count = grid_size * grid_size;
    m_context->draw_indexed(
        &cmd_list, static_cast<unsigned>(m_bevel_cube_mesh.index.first.count), bevel_instance_count, 0, 0, 0);

    qhenki::gfx::ImageBarrier rt_to_srv{
        .src_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
        .src_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
    };
    m_context->set_barrier_resource(1, &rt_to_srv, m_offscreen_texture.tex);
    m_context->issue_barrier(&cmd_list, 1, &rt_to_srv);

    qhenki::gfx::RenderTarget blit_target{
        .clear_params = {.clear_color_value = {0.f, 0.f, 0.f, 0.f}},
        .clear_type = qhenki::gfx::RenderTarget::COLOR,
        .descriptor = m_bloom_textures[m_starting_bloom_index].rt_descriptor,
    };
    m_context->start_render_pass(&cmd_list, 1, qhenki::util::ptr_array(blit_target).data(), nullptr);
    const unsigned bloom_w = m_bloom_textures.front().tex.desc.width;
    const unsigned bloom_h = m_bloom_textures.front().tex.desc.height;
    const D3D12_VIEWPORT bloom_viewport{
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = static_cast<float>(bloom_w),
        .Height = static_cast<float>(bloom_h),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };
    const D3D12_RECT bloom_scissor{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(bloom_w),
        .bottom = static_cast<LONG>(bloom_h),
    };
    m_context->set_viewports(&cmd_list, 1, &bloom_viewport);
    m_context->set_scissor_rects(&cmd_list, 1, &bloom_scissor);

    // Pick out bright pixels into low resolution FB
    m_context->bind_pipeline(&cmd_list, m_blit_luminance_pipeline);
    if (m_context->is_compatibility())
    {
        m_context->compatibility_set_textures(1,
                                              1,
                                              qhenki::util::ptr_array(m_offscreen_texture.srv_descriptor).data(),
                                              qhenki::gfx::ACCESS_SHADER_RESOURCE,
                                              qhenki::gfx::PipelineStage::PIXEL);
    }
    else
    {
        qhenki::gfx::Descriptor descriptor{.heap = &m_GPU_heap, .offset = gpu_descriptor_heap_index++};
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_offscreen_texture.srv_descriptor, descriptor));
        m_context->set_descriptor_table(&cmd_list, 1, descriptor);
    }

    m_context->draw(&cmd_list, 3, 0);

    // Start blur passes
    size_t blur_srv_start = gpu_descriptor_heap_index;
    if (!m_context->is_compatibility())
    {
        gpu_descriptor_heap_index += 2;
        THROW_IF_FALSE(m_bloom_textures[1].srv_descriptor.offset - m_bloom_textures[0].srv_descriptor.offset == 1);
        qhenki::gfx::Descriptor descriptor{.heap = &m_GPU_heap, .offset = blur_srv_start};
        THROW_IF_FALSE(m_context->copy_descriptors(2, m_bloom_textures[0].srv_descriptor, descriptor));
    }
    constexpr unsigned BLUR_PASSES = 4;
    const unsigned iterations = BLUR_PASSES * 2;
    for (unsigned i = 0; i <= iterations; i++)
    {
        // RT -> SRV, SRV -> RT, and if on last include Present -> RT for final composition
        std::array<qhenki::gfx::ImageBarrier, 3> swap_barriers;
        // RT -> SRV
        swap_barriers[0] = {
            .src_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
            .dst_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,
            .src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
            .dst_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
            .src_layout = qhenki::gfx::Layout::RENDER_TARGET,
            .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
        };
        auto& bloom0 = m_bloom_textures[m_starting_bloom_index];
        m_context->set_barrier_resource(1, &swap_barriers[0], bloom0.tex);
        // SRV -> RT
        swap_barriers[1] = {
            .src_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,
            .dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
            .src_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
            .dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
            .src_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
            .dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
        };
        auto& bloom1 = m_bloom_textures[1 - m_starting_bloom_index];
        m_context->set_barrier_resource(1, &swap_barriers[1], bloom1.tex);
        // Present -> RT
        swap_barriers[2] = {
            .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW,
            .dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
            .src_access = qhenki::gfx::AccessFlags::ACCESS_COMMON,
            .dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
            .src_layout = qhenki::gfx::Layout::PRESENT,
            .dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
        };
        m_context->set_barrier_resource(1, &swap_barriers[2], m_swapchain, m_frame_index);

        m_starting_bloom_index = (m_starting_bloom_index + 1) % m_bloom_textures.size();

        if (i == iterations)
        {
            m_context->issue_barrier(&cmd_list, 3, swap_barriers.data());
            break;
        }
        m_context->issue_barrier(&cmd_list, 2, swap_barriers.data());

        qhenki::gfx::RenderTarget blit_target_blur{
            .clear_params = {.clear_color_value = {0.f, 0.f, 0.f, 0.f}},
            .clear_type = qhenki::gfx::RenderTarget::COLOR,
            .descriptor = bloom1.rt_descriptor,
        };
        m_context->start_render_pass(&cmd_list, 1, qhenki::util::ptr_array(blit_target_blur).data(), nullptr);
        m_context->set_viewports(&cmd_list, 1, &bloom_viewport);
        m_context->set_scissor_rects(&cmd_list, 1, &bloom_scissor);

        const bool horizontal = i % 2 == 0;
        m_context->bind_pipeline(&cmd_list,
                                 horizontal ? m_blit_bloom_1d_horizontal_pipeline : m_blit_bloom_1d_vertical_pipeline);

        if (m_context->is_compatibility())
        {
            m_context->compatibility_set_textures(
                1,
                2,
                qhenki::util::ptr_array(bloom0.srv_descriptor, bloom0.srv_descriptor).data(),
                qhenki::gfx::ACCESS_SHADER_RESOURCE,
                qhenki::gfx::PipelineStage::PIXEL);
        }
        else
        {
            m_context->set_descriptor_table(
                &cmd_list,
                1,
                qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = blur_srv_start + (1 - m_starting_bloom_index)});
        }

        m_context->draw(&cmd_list, 3, 0);
    }

    // Composite image into swapchain backbuffer
    std::array clear_values = {0.f, 0.f, 0.f, 1.f};
    m_context->start_render_pass(&cmd_list, &m_swapchain, clear_values.data(), nullptr, m_frame_index);
    m_context->set_viewports(&cmd_list, 1, &viewport);
    m_context->set_scissor_rects(&cmd_list, 1, &scissor_rect);
    m_context->bind_pipeline(&cmd_list, m_blit_copy_pipeline);
    auto& final_bloom = m_bloom_textures[1 - m_starting_bloom_index];
    if (m_context->is_compatibility())
    {
        m_context->compatibility_set_textures(
            1,
            2,
            qhenki::util::ptr_array(m_offscreen_texture.srv_descriptor, final_bloom.srv_descriptor).data(),
            qhenki::gfx::ACCESS_SHADER_RESOURCE,
            qhenki::gfx::PipelineStage::PIXEL);
    }
    else
    {
        auto composite_start = gpu_descriptor_heap_index;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_offscreen_texture.srv_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap,
                                                                           .offset = gpu_descriptor_heap_index++}));
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   final_bloom.srv_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap,
                                                                           .offset = gpu_descriptor_heap_index++}));
        m_context->set_descriptor_table(&cmd_list,
                                        1,
                                        qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = composite_start});
    }
    m_context->draw(&cmd_list, 3, 0);

    ImGui::Render();
    m_context->render_imgui_draw_data(&cmd_list);

    std::array<qhenki::gfx::ImageBarrier, 2> end_barriers;
    // RT -> Present for swapchain
    end_barriers[0] = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_DRAW,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_NONE,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .dst_access = qhenki::gfx::AccessFlags::NO_ACCESS,
        .src_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .dst_layout = qhenki::gfx::Layout::PRESENT,
    };
    m_context->set_barrier_resource(1, &end_barriers[0], m_swapchain, m_frame_index);
    // Offscreen back to RT
    end_barriers[1] = {
        .src_stage = qhenki::gfx::SyncStage::SYNC_PIXEL_SHADING,
        .dst_stage = qhenki::gfx::SyncStage::SYNC_RENDER_TARGET,
        .src_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,
        .dst_access = qhenki::gfx::AccessFlags::ACCESS_RENDER_TARGET,
        .src_layout = qhenki::gfx::Layout::SHADER_RESOURCE,
        .dst_layout = qhenki::gfx::Layout::RENDER_TARGET,
    };
    m_context->set_barrier_resource(1, &end_barriers[1], m_offscreen_texture.tex);
    m_context->issue_barrier(&cmd_list, end_barriers.size(), end_barriers.data());

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
        .format = m_depth_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
    };
    THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, "Depth Buffer Texture"));
    THROW_IF_FALSE(m_context->create_descriptor_depth_stencil(m_depth_buffer, &m_dsv_heap, &m_depth_buffer_descriptor));

    const qhenki::gfx::TextureDesc offscreen_rt_desc{
        .width = static_cast<uint64_t>(width),
        .height = static_cast<uint32_t>(height),
        .format = m_offscreen_rt_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .is_render_target = true,
    };
    THROW_IF_FALSE(m_context->create_texture(offscreen_rt_desc, &m_offscreen_texture.tex, "Offscreen RT"));
    THROW_IF_FALSE(m_context->create_descriptor_render_target(m_offscreen_texture.tex,
                                                              &m_rtv_heap,
                                                              &m_offscreen_texture.rt_descriptor));
    THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_offscreen_texture.tex,
                                                            &m_CPU_heap,
                                                            &m_offscreen_texture.srv_descriptor));

    qhenki::gfx::TextureDesc blit_rt_desc{
        .width = static_cast<uint64_t>(width / 4),
        .height = static_cast<uint32_t>(height / 4),
        .format = m_offscreen_rt_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .is_render_target = true,
    };
    for (unsigned i = 0; i < m_bloom_textures.size(); i++)
    {
        blit_rt_desc.initial_layout = i == m_starting_bloom_index ? qhenki::gfx::Layout::RENDER_TARGET
                                                                  : qhenki::gfx::Layout::SHADER_RESOURCE;
        THROW_IF_FALSE(m_context->create_texture(blit_rt_desc,
                                                 &m_bloom_textures[i].tex,
                                                 qhenki::util::format_string("Bloom RT %u", i).buffer.data()));
        THROW_IF_FALSE(m_context->create_descriptor_render_target(m_bloom_textures[i].tex,
                                                                  &m_rtv_heap,
                                                                  &m_bloom_textures[i].rt_descriptor));
        THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_bloom_textures[i].tex,
                                                                &m_CPU_heap,
                                                                &m_bloom_textures[i].srv_descriptor));
    }
}

void RetroExampleApp::destroy()
{
    m_context->destroy_imgui();
}

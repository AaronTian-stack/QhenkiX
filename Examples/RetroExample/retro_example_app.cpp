#include "retro_example_app.h"
#include "example_shared/macros.h"
#include "example_shared/shader_loader.h"
#include "example_shared/window_init.h"
#include "shared_structs.h"

#include <DirectXTex.h>

#include <imgui/imgui.h>

#include <qhenki/utility/general_util.h>
#include <qhenki/utility/gfx_util.h>
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
    init_display_window_with_name(*this, m_window, "Retro Example", payload);
}

void RetroExampleApp::create()
{
    const auto api = get_graphics_api();
    const bool use_dx11 = api == qhenki::gfx::API::D3D11;

    const auto select_profile_base = [&](const char* shader_model_5_0, const char* shader_model_6_6)
    {
        return use_dx11 ? shader_model_5_0 : shader_model_6_6;
    };

    qhenki::gfx::LayoutBinding b1{
        .binding = 0,
        .count = 1,
        .type = qhenki::gfx::LayoutBinding::CBV,
    };
    qhenki::gfx::LayoutBinding b2{
        .binding = 1,
        .count = 2,
        .type = qhenki::gfx::LayoutBinding::SRV,
    };
    qhenki::gfx::LayoutBinding b3{
        .binding = 0,
        .count = 2,
        .type = qhenki::gfx::LayoutBinding::SAMPLER,
    };
    qhenki::gfx::PipelineLayoutDesc layout_desc{};
    layout_desc.spaces[0] = {b1}; // CBV
    layout_desc.spaces[1] = {b2}; // SRV
    layout_desc.spaces[2] = {b3}; // Samplers
    THROW_IF_FALSE(m_context->create_pipeline_layout(&layout_desc, &m_pipeline_layout));

    qhenki::gfx::DescriptorHeapDesc heap_desc_GPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .num_descriptors = 256,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_GPU, &m_GPU_heap, "GPU heap"));

    qhenki::gfx::DescriptorHeapDesc heap_desc_CPU{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::CBV_SRV_UAV,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::CPU,
        .num_descriptors = heap_desc_GPU.num_descriptors,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(heap_desc_CPU, &m_CPU_heap, "CPU heap"));

    qhenki::gfx::DescriptorHeapDesc sampler_heap_desc{
        .type = qhenki::gfx::DescriptorHeapDesc::Type::SAMPLER,
        .visibility = qhenki::gfx::DescriptorHeapDesc::Visibility::GPU,
        .num_descriptors = 16,
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(sampler_heap_desc, &m_sampler_heap, "Sampler heap"));

    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        THROW_IF_FALSE(m_context->create_command_pool(&m_cmd_pools[i], qhenki::gfx::GRAPHICS));
        THROW_IF_FALSE(m_context->create_command_list(&m_cmd_lists[i], m_cmd_pools[i]));
    }

    qhenki::gfx::BufferDesc matrix_desc{.size = sizeof(FrameConstants),
                                        .usage = qhenki::gfx::BufferUsage::CONSTANT,
                                        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL};
    for (unsigned i = 0; i < m_frames_in_flight; i++)
    {
        m_matrix_descriptors[i].offset = i;
        THROW_IF_FALSE(m_context->create_buffer(matrix_desc, nullptr, &m_matrix_buffers[i], "Frame Constant Buffer"));
        THROW_IF_FALSE(
            m_context->create_descriptor_constant_view(m_matrix_buffers[i], &m_CPU_heap, &m_matrix_descriptors[i]));
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    auto set_accessor = [&model, this](Mesh::AccessorBufferView* abv, const int accessor_idx)
    {
        const auto& accessor = model.accessors[accessor_idx];
        abv->accessor = {
            .offset = accessor.byteOffset,
            .count = accessor.count,
            .type = accessor.type,
            .component_type = accessor.componentType,
        };
        const auto& buffer_view = model.bufferViews[accessor.bufferView];
        abv->buffer_view = {
            .offset = buffer_view.byteOffset,
            .length = buffer_view.byteLength,
            .stride = buffer_view.byteStride,
        };
    };

    THROW_IF_FALSE(loader.LoadBinaryFromFile(&model, &err, &warn, "assets/cylinder.glb"));
    assert(model.meshes.size() == 1);
    assert(model.meshes[0].primitives.size() == 1);
    auto& prim = model.meshes[0].primitives[0];

    set_accessor(&m_skybox_mesh.position, prim.attributes.at("POSITION"));
    set_accessor(&m_skybox_mesh.index, prim.indices);

    assert(model.buffers.size() == 1);
    const size_t skybox_buffer_bytes = model.buffers[0].data.size();

    tinygltf::Model stencil_model;
    THROW_IF_FALSE(loader.LoadBinaryFromFile(&stencil_model, &err, &warn, "assets/cylinder_capped.glb"));
    assert(stencil_model.meshes.size() == 1);
    assert(stencil_model.meshes[0].primitives.size() == 1);
    auto& stencil_prim = stencil_model.meshes[0].primitives[0];

    auto set_accessor_stencil = [&stencil_model](Mesh::AccessorBufferView* abv, const int accessor_idx)
    {
        const auto& accessor = stencil_model.accessors[accessor_idx];
        abv->accessor = {
            .offset = accessor.byteOffset,
            .count = accessor.count,
            .type = accessor.type,
            .component_type = accessor.componentType,
        };
        const auto& buffer_view = stencil_model.bufferViews[accessor.bufferView];
        abv->buffer_view = {
            .offset = buffer_view.byteOffset,
            .length = buffer_view.byteLength,
            .stride = buffer_view.byteStride,
        };
    };
    set_accessor_stencil(&m_stencil_mesh.position, stencil_prim.attributes.at("POSITION"));
    set_accessor_stencil(&m_stencil_mesh.index, stencil_prim.indices);

    assert(stencil_model.buffers.size() == 1);
    const size_t stencil_buffer_bytes = stencil_model.buffers[0].data.size();

    tinygltf::Model cube_model;
    THROW_IF_FALSE(loader.LoadBinaryFromFile(&cube_model, &err, &warn, "assets/bevel_cube.glb"));
    assert(cube_model.meshes.size() == 1);
    assert(cube_model.meshes[0].primitives.size() == 1);
    auto& cube_prim = cube_model.meshes[0].primitives[0];

    auto set_accessor_cube = [&cube_model](Mesh::AccessorBufferView* abv, const int accessor_idx)
    {
        const auto& accessor = cube_model.accessors[accessor_idx];
        abv->accessor = {
            .offset = accessor.byteOffset,
            .count = accessor.count,
            .type = accessor.type,
            .component_type = accessor.componentType,
        };
        const auto& buffer_view = cube_model.bufferViews[accessor.bufferView];
        abv->buffer_view = {
            .offset = buffer_view.byteOffset,
            .length = buffer_view.byteLength,
            .stride = buffer_view.byteStride,
        };
    };
    set_accessor_cube(&m_bevel_cube_mesh.position, cube_prim.attributes.at("POSITION"));
    set_accessor_cube(&m_bevel_cube_mesh.normal, cube_prim.attributes.at("NORMAL"));
    set_accessor_cube(&m_bevel_cube_mesh.index, cube_prim.indices);

    assert(cube_model.buffers.size() == 1);
    const size_t cube_buffer_bytes = cube_model.buffers[0].data.size();

    qhenki::gfx::BufferDesc gpu_mesh_desc{
        .size = skybox_buffer_bytes,
        .usage = qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX |
                 qhenki::gfx::BufferUsage::COPY_DST,
        .visibility = qhenki::gfx::BufferVisibility::GPU,
    };
    THROW_IF_FALSE(m_context->create_buffer(gpu_mesh_desc, nullptr, &m_skybox_buffer, "Skybox Vertex Buffer"));

    gpu_mesh_desc.size = stencil_buffer_bytes;
    THROW_IF_FALSE(m_context->create_buffer(gpu_mesh_desc, nullptr, &m_stencil_mesh.buffer, "Stencil cylinder"));

    gpu_mesh_desc.size = cube_buffer_bytes;
    THROW_IF_FALSE(m_context->create_buffer(gpu_mesh_desc, nullptr, &m_bevel_cube_mesh.buffer, "Bevel Cube"));

    qhenki::gfx::SamplerDesc sampler_desc{
        .min_filter = qhenki::gfx::Filter::NEAREST,
        .mag_filter = qhenki::gfx::Filter::NEAREST,
        .mip_filter = qhenki::gfx::Filter::NEAREST,
        .address_mode_u = qhenki::gfx::AddressMode::WRAP,
        .address_mode_v = qhenki::gfx::AddressMode::WRAP,
        .address_mode_w = qhenki::gfx::AddressMode::WRAP,
    };
    m_sampler_descriptor.offset = 0;
    THROW_IF_FALSE(m_context->create_descriptor(sampler_desc, &m_sampler_heap, &m_sampler_descriptor));

    qhenki::gfx::SamplerDesc sampler_linear_desc{
        .min_filter = qhenki::gfx::Filter::LINEAR,
        .mag_filter = qhenki::gfx::Filter::LINEAR,
        .mip_filter = qhenki::gfx::Filter::LINEAR,
        .address_mode_u = qhenki::gfx::AddressMode::CLAMP,
        .address_mode_v = qhenki::gfx::AddressMode::CLAMP,
        .address_mode_w = qhenki::gfx::AddressMode::CLAMP,
    };
    m_sampler_linear_descriptor.offset = 1;
    THROW_IF_FALSE(m_context->create_descriptor(sampler_linear_desc, &m_sampler_heap, &m_sampler_linear_descriptor));

    const wchar_t* skybox_path = L"assets/skybox.dds";
    ScratchImage scratch;
    TexMetadata meta;
    const auto hr = LoadFromDDSFile(skybox_path, DDS_FLAGS_NONE, &meta, scratch);
    THROW_IF_TRUE(FAILED(hr));
    assert(meta.width <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()));
    qhenki::gfx::TextureDesc skybox_tex_desc{
        .width = static_cast<uint32_t>(meta.width),
        .height = static_cast<uint32_t>(meta.height),
        .depth_or_array_size = static_cast<uint16_t>(meta.arraySize),
        .mip_levels = static_cast<uint16_t>(meta.mipLevels),
        .format = qhenki::gfx::format_from_dxgi(meta.format),
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .is_cube = meta.IsCubemap(),
        .initial_layout = qhenki::gfx::Layout::COPY_DEST,
        .usage = qhenki::gfx::TextureDesc::COPY_DEST | qhenki::gfx::TextureDesc::SHADER_RESOURCE,
    };
    THROW_IF_FALSE(m_context->create_texture(skybox_tex_desc, &m_skybox_texture, "Skybox Texture"));

    m_skybox_texture_descriptor.offset = m_frames_in_flight;
    THROW_IF_FALSE(
        m_context->create_descriptor_shader_view(m_skybox_texture, &m_CPU_heap, &m_skybox_texture_descriptor));

    const auto geometry_bytes = skybox_buffer_bytes + stencil_buffer_bytes + cube_buffer_bytes;
    const auto texture_start = qhenki::util::align_up(geometry_bytes,
                                                      m_context->get_staging_alignment(m_skybox_texture));
    const auto staging_size = texture_start + m_context->get_required_staging_size(m_skybox_texture);

    qhenki::gfx::Buffer upload_staging{};
    qhenki::gfx::BufferDesc upload_staging_desc{
        .size = staging_size,
        .usage = qhenki::gfx::BufferUsage::COPY_SRC,
        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL,
    };
    THROW_IF_FALSE(m_context->create_buffer(upload_staging_desc, nullptr, &upload_staging, "Upload staging"));

    const unsigned frame_slot_init = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[frame_slot_init]));
    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[frame_slot_init], m_cmd_pools[frame_slot_init]));
    auto& cmd_list_init = m_cmd_lists[frame_slot_init];

    {
        void* const ptr = m_context->map_buffer(upload_staging);
        THROW_IF_FALSE(ptr);
        auto* const bytes = static_cast<std::byte*>(ptr);
        memcpy(bytes, model.buffers[0].data.data(), skybox_buffer_bytes);
        memcpy(bytes + skybox_buffer_bytes, stencil_model.buffers[0].data.data(), stencil_buffer_bytes);
        memcpy(bytes + skybox_buffer_bytes + stencil_buffer_bytes,
               cube_model.buffers[0].data.data(),
               cube_buffer_bytes);
        m_context->unmap_buffer(upload_staging);
    }

    m_context->copy_buffer(&cmd_list_init, upload_staging, 0, &m_skybox_buffer, 0, skybox_buffer_bytes);
    m_context->copy_buffer(
        &cmd_list_init, upload_staging, skybox_buffer_bytes, &m_stencil_mesh.buffer, 0, stencil_buffer_bytes);
    m_context->copy_buffer(&cmd_list_init,
                           upload_staging,
                           skybox_buffer_bytes + stencil_buffer_bytes,
                           &m_bevel_cube_mesh.buffer,
                           0,
                           cube_buffer_bytes);

    const qhenki::gfx::BufferRange texture_staging_range{
        .buffer = &upload_staging,
        .offset = texture_start,
    };
    THROW_IF_FALSE(
        m_context->copy_to_texture(&cmd_list_init, scratch.GetPixels(), texture_staging_range, &m_skybox_texture));

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
    auto current_fence_value = ++m_fence_frame_ready_val[frame_slot_init];
    qhenki::gfx::SubmitInfo info_init{
        .command_list_count = 1,
        .command_lists = &cmd_list_init,
        .signal_fence_count = 1,
        .signal_fences = &m_fence_frame_ready,
        .signal_values = &current_fence_value,
    };
    m_context->submit_command_lists(info_init, qhenki::gfx::QueueType::GRAPHICS);

    qhenki::gfx::WaitInfo wait_info{.count = 1,
                                    .fences = &m_fence_frame_ready,
                                    .values = &m_fence_frame_ready_val[frame_slot_init]};
    THROW_IF_FALSE(m_context->wait_fences(wait_info));

    // Load skybox shaders
    char skybox_vs_name[96]{};
    char skybox_ps_name[96]{};
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("skybox_vs_5_0_vs_main", "skybox_vs_6_6_vs_main"),
                                           skybox_vs_name,
                                           sizeof(skybox_vs_name)));
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("skybox_ps_5_0_ps_main", "skybox_ps_6_6_ps_main"),
                                           skybox_ps_name,
                                           sizeof(skybox_ps_name)));

    uPtr<std::byte, void (*)(void*)> skybox_vs_data(nullptr, free);
    qhenki::gfx::Shader skybox_vertex_shader;
    read_compiled_shader_bytes(api, skybox_vs_name, &skybox_vs_data, &skybox_vertex_shader.size);
    skybox_vertex_shader.data = skybox_vs_data.get();

    uPtr<std::byte, void (*)(void*)> skybox_ps_data(nullptr, free);
    qhenki::gfx::Shader skybox_pixel_shader;
    read_compiled_shader_bytes(api, skybox_ps_name, &skybox_ps_data, &skybox_pixel_shader.size);
    skybox_pixel_shader.data = skybox_ps_data.get();

    qhenki::gfx::BlendDesc skybox_blend_desc{
        .alpha_to_coverage_enable = false,
        .independent_blend_enable = false,
        .render_target =
            {
                {
                    .blend_enable = true,
                    .logic_op_enable = false,
                    .src_blend = qhenki::gfx::Blend::SRC_ALPHA,
                    .dst_blend = qhenki::gfx::Blend::INV_SRC_ALPHA,
                    .blend_op = qhenki::gfx::BlendOp::ADD,
                    .src_blend_alpha = qhenki::gfx::Blend::ONE,
                    .dst_blend_alpha = qhenki::gfx::Blend::INV_SRC_ALPHA,
                    .blend_op_alpha = qhenki::gfx::BlendOp::ADD,
                    .logic_op = qhenki::gfx::LogicOp::NOOP,
                    .render_target_write_mask = 0xF,
                },
            },
    };
    qhenki::gfx::DepthStencilDesc skybox_depth_desc{};
    skybox_depth_desc.depth_func = qhenki::gfx::ComparisonFunc::LESS_OR_EQUAL;
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
                                              skybox_vertex_shader,
                                              skybox_pixel_shader,
                                              &m_pipeline_layout,
                                              "Skybox pipeline"));

    // Load cube shaders
    char cube_vs_name[96]{};
    char cube_ps_name[96]{};
    THROW_IF_FALSE(append_shader_extension(
        api, select_profile_base("cube_vs_5_0_vs_main", "cube_vs_6_6_vs_main"), cube_vs_name, sizeof(cube_vs_name)));
    THROW_IF_FALSE(append_shader_extension(
        api, select_profile_base("cube_ps_5_0_ps_main", "cube_ps_6_6_ps_main"), cube_ps_name, sizeof(cube_ps_name)));

    uPtr<std::byte, void (*)(void*)> cube_vs_data(nullptr, free);
    qhenki::gfx::Shader cube_vertex_shader;
    read_compiled_shader_bytes(api, cube_vs_name, &cube_vs_data, &cube_vertex_shader.size);
    cube_vertex_shader.data = cube_vs_data.get();

    uPtr<std::byte, void (*)(void*)> cube_ps_data(nullptr, free);
    qhenki::gfx::Shader cube_pixel_shader;
    read_compiled_shader_bytes(api, cube_ps_name, &cube_ps_data, &cube_pixel_shader.size);
    cube_pixel_shader.data = cube_ps_data.get();

    qhenki::gfx::GraphicsPipelineDesc cube_pipeline_desc = {
        .depth_stencil_state = qhenki::gfx::DepthStencilDesc{},
        .rtv_formats = {m_offscreen_rt_format},
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(cube_pipeline_desc,
                                              &m_cube_pipeline,
                                              cube_vertex_shader,
                                              cube_pixel_shader,
                                              &m_pipeline_layout,
                                              "Cube pipeline"));

    // Load stencil shaders
    char stencil_vs_name[96]{};
    char stencil_ps_name[96]{};
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("stencil_vs_5_0_vs_main", "stencil_vs_6_6_vs_main"),
                                           stencil_vs_name,
                                           sizeof(stencil_vs_name)));
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("stencil_ps_5_0_ps_main", "stencil_ps_6_6_ps_main"),
                                           stencil_ps_name,
                                           sizeof(stencil_ps_name)));

    uPtr<std::byte, void (*)(void*)> stencil_vs_data(nullptr, free);
    qhenki::gfx::Shader stencil_vertex_shader;
    read_compiled_shader_bytes(api, stencil_vs_name, &stencil_vs_data, &stencil_vertex_shader.size);
    stencil_vertex_shader.data = stencil_vs_data.get();

    uPtr<std::byte, void (*)(void*)> stencil_ps_data(nullptr, free);
    qhenki::gfx::Shader stencil_pixel_shader;
    read_compiled_shader_bytes(api, stencil_ps_name, &stencil_ps_data, &stencil_pixel_shader.size);
    stencil_pixel_shader.data = stencil_ps_data.get();

    qhenki::gfx::BlendDesc stencil_blend_desc{
        .alpha_to_coverage_enable = false,
        .independent_blend_enable = false,
        .render_target =
            {
                {
                    .blend_enable = false,
                    .logic_op_enable = false,
                    .src_blend = qhenki::gfx::Blend::ONE,
                    .dst_blend = qhenki::gfx::Blend::ZERO,
                    .blend_op = qhenki::gfx::BlendOp::ADD,
                    .src_blend_alpha = qhenki::gfx::Blend::ONE,
                    .dst_blend_alpha = qhenki::gfx::Blend::ZERO,
                    .blend_op_alpha = qhenki::gfx::BlendOp::ADD,
                    .logic_op = qhenki::gfx::LogicOp::NOOP,
                    .render_target_write_mask = 0,
                },
            },
    };

    qhenki::gfx::GraphicsPipelineDesc stencil_pipeline_desc = {
        .blend_desc = stencil_blend_desc,
        .depth_stencil_state =
            qhenki::gfx::DepthStencilDesc{
                .front_face =
                    {
                        .fail_op = qhenki::gfx::StencilOp::KEEP,
                        .depth_fail_op = qhenki::gfx::StencilOp::KEEP,
                        .pass_op = qhenki::gfx::StencilOp::INCREMENT_AND_CLAMP,
                        .func = qhenki::gfx::ComparisonFunc::ALWAYS,
                    },
                .back_face =
                    {
                        .fail_op = qhenki::gfx::StencilOp::KEEP,
                        .depth_fail_op = qhenki::gfx::StencilOp::KEEP,
                        .pass_op = qhenki::gfx::StencilOp::INCREMENT_AND_CLAMP,
                        .func = qhenki::gfx::ComparisonFunc::ALWAYS,
                    },
                .depth_write_enable = false,
                .depth_func = qhenki::gfx::ComparisonFunc::LESS_OR_EQUAL,
                .stencil_read_mask = 0xFF,
                .stencil_write_mask = 0xFF,
                .depth_enable = true,
                .stencil_enable = true,
            },
        .rtv_formats = {m_offscreen_rt_format},
        .rasterizer_state =
            qhenki::gfx::RasterizerDesc{
                .cull_mode = qhenki::gfx::CullMode::BACK,
                .front_counter_clockwise = false,
            },
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = true,
    };
    THROW_IF_FALSE(m_context->create_pipeline(stencil_pipeline_desc,
                                              &m_stencil_pipeline,
                                              stencil_vertex_shader,
                                              stencil_pixel_shader,
                                              &m_pipeline_layout,
                                              "Stencil cube pipeline"));

    // Load bevel cube (instanced) shaders
    char bevel_vs_name[96]{};
    char bevel_ps_name[96]{};
    THROW_IF_FALSE(
        append_shader_extension(api,
                                select_profile_base("cube_instanced_vs_5_0_vs_main", "cube_instanced_vs_6_6_vs_main"),
                                bevel_vs_name,
                                sizeof(bevel_vs_name)));
    THROW_IF_FALSE(
        append_shader_extension(api,
                                select_profile_base("cube_instanced_ps_5_0_ps_main", "cube_instanced_ps_6_6_ps_main"),
                                bevel_ps_name,
                                sizeof(bevel_ps_name)));

    uPtr<std::byte, void (*)(void*)> bevel_vs_data(nullptr, free);
    qhenki::gfx::Shader bevel_vertex_shader;
    read_compiled_shader_bytes(api, bevel_vs_name, &bevel_vs_data, &bevel_vertex_shader.size);
    bevel_vertex_shader.data = bevel_vs_data.get();

    uPtr<std::byte, void (*)(void*)> bevel_ps_data(nullptr, free);
    qhenki::gfx::Shader bevel_pixel_shader;
    read_compiled_shader_bytes(api, bevel_ps_name, &bevel_ps_data, &bevel_pixel_shader.size);
    bevel_pixel_shader.data = bevel_ps_data.get();
    qhenki::gfx::GraphicsPipelineDesc bevel_pipeline_desc = {
        .depth_stencil_state =
            qhenki::gfx::DepthStencilDesc{
                .front_face =
                    {
                        .fail_op = qhenki::gfx::StencilOp::KEEP,
                        .depth_fail_op = qhenki::gfx::StencilOp::KEEP,
                        .pass_op = qhenki::gfx::StencilOp::KEEP,
                        .func = qhenki::gfx::ComparisonFunc::EQUAL,
                    },
                .back_face =
                    {
                        .fail_op = qhenki::gfx::StencilOp::KEEP,
                        .depth_fail_op = qhenki::gfx::StencilOp::KEEP,
                        .pass_op = qhenki::gfx::StencilOp::KEEP,
                        .func = qhenki::gfx::ComparisonFunc::EQUAL,
                    },
                .depth_write_enable = true,
                .depth_func = qhenki::gfx::ComparisonFunc::LESS_OR_EQUAL,
                .stencil_read_mask = 0xFF,
                .stencil_write_mask = 0x00,
                .depth_enable = true,
                .stencil_enable = true,
            },
        .rtv_formats = {m_offscreen_rt_format},
        .rasterizer_state =
            qhenki::gfx::RasterizerDesc{
                .cull_mode = qhenki::gfx::CullMode::BACK,
                .front_counter_clockwise = false,
            },
        .num_render_targets = 1,
        .dsv_format = m_depth_format,
        .increment_slot = true,
    };

    THROW_IF_FALSE(m_context->create_pipeline(bevel_pipeline_desc,
                                              &m_bevel_cube_pipeline,
                                              bevel_vertex_shader,
                                              bevel_pixel_shader,
                                              &m_pipeline_layout,
                                              "Bevel cube instanced pipeline"));

    // Load fullscreen triangle (blit) vertex shader
    char blit_vs_name[96]{};
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("fullscreen_triangle_vs_5_0_vs_main",
                                                               "fullscreen_triangle_vs_6_6_vs_main"),
                                           blit_vs_name,
                                           sizeof(blit_vs_name)));

    uPtr<std::byte, void (*)(void*)> blit_vs_data(nullptr, free);
    qhenki::gfx::Shader blit_vertex_shader;
    read_compiled_shader_bytes(api, blit_vs_name, &blit_vs_data, &blit_vertex_shader.size);
    blit_vertex_shader.data = blit_vs_data.get();

    // Load blit copy pixel shader
    char blit_copy_ps_name[96]{};
    THROW_IF_FALSE(append_shader_extension(api,
                                           select_profile_base("blit_copy_ps_5_0_ps_main", "blit_copy_ps_6_6_ps_main"),
                                           blit_copy_ps_name,
                                           sizeof(blit_copy_ps_name)));

    uPtr<std::byte, void (*)(void*)> blit_copy_ps_data(nullptr, free);
    qhenki::gfx::Shader blit_copy_pixel_shader;
    read_compiled_shader_bytes(api, blit_copy_ps_name, &blit_copy_ps_data, &blit_copy_pixel_shader.size);
    blit_copy_pixel_shader.data = blit_copy_ps_data.get();

    // Load blit luminance pixel shader
    char blit_luminance_ps_name[96]{};
    THROW_IF_FALSE(
        append_shader_extension(api,
                                select_profile_base("blit_luminance_ps_5_0_ps_main", "blit_luminance_ps_6_6_ps_main"),
                                blit_luminance_ps_name,
                                sizeof(blit_luminance_ps_name)));

    uPtr<std::byte, void (*)(void*)> blit_luminance_ps_data(nullptr, free);
    qhenki::gfx::Shader blit_luminance_pixel_shader;
    read_compiled_shader_bytes(api, blit_luminance_ps_name, &blit_luminance_ps_data, &blit_luminance_pixel_shader.size);
    blit_luminance_pixel_shader.data = blit_luminance_ps_data.get();
    {
        uPtr<std::byte, void (*)(void*)> blob_data(nullptr, free);
        size_t blob_size = 0;

        const char* blit_bloom_base = select_profile_base("blit_bloom_1d_ps_5_0_ps_main",
                                                          "blit_bloom_1d_ps_6_6_ps_main");
        char blit_bloom_name[96]{};
        THROW_IF_FALSE(append_shader_extension(api, blit_bloom_base, blit_bloom_name, sizeof(blit_bloom_name)));
        const auto blit_bloom_blob_name = qhenki::util::format_string("%s_blob", blit_bloom_name);

        THROW_IF_FALSE(read_compiled_shader_bytes(api, blit_bloom_blob_name.buffer.data(), &blob_data, &blob_size));
        const char* horizontal_defines[] = {"BLUR_HORIZONTAL=1"};
        const char* vertical_defines[] = {"BLUR_HORIZONTAL=0"};
        void* horizontal_shader_ptr = nullptr;
        size_t horizontal_shader_size = 0;
        void* vertical_shader_ptr = nullptr;
        size_t vertical_shader_size = 0;
        THROW_IF_FALSE(qhenki::util::find_permutation_in_blob(
            blob_data.get(), blob_size, horizontal_defines, 1, &horizontal_shader_ptr, &horizontal_shader_size));
        THROW_IF_FALSE(qhenki::util::find_permutation_in_blob(
            blob_data.get(), blob_size, vertical_defines, 1, &vertical_shader_ptr, &vertical_shader_size));

        qhenki::gfx::Shader blit_bloom_1d_horizontal_pixel_shader{
            .data = horizontal_shader_ptr,
            .size = horizontal_shader_size,
        };
        qhenki::gfx::Shader blit_bloom_1d_vertical_pixel_shader{
            .data = vertical_shader_ptr,
            .size = vertical_shader_size,
        };

        qhenki::gfx::GraphicsPipelineDesc blit_pipeline_desc = {
            .rtv_formats = {m_offscreen_rt_format},
            .num_render_targets = 1,
            .dsv_format = qhenki::gfx::Format::UNKNOWN,
            .increment_slot = true,
        };
        qhenki::gfx::GraphicsPipelineDesc blit_copy_pipeline_desc = {
            .rtv_formats = {m_swapchain.format},
            .num_render_targets = 1,
            .dsv_format = qhenki::gfx::Format::UNKNOWN,
            .increment_slot = true,
        };
        THROW_IF_FALSE(m_context->create_pipeline(blit_copy_pipeline_desc,
                                                  &m_blit_copy_pipeline,
                                                  blit_vertex_shader,
                                                  blit_copy_pixel_shader,
                                                  &m_pipeline_layout,
                                                  "Blit copy pipeline"));
        THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                                  &m_blit_luminance_pipeline,
                                                  blit_vertex_shader,
                                                  blit_luminance_pixel_shader,
                                                  &m_pipeline_layout,
                                                  "Blit luminance pipeline"));
        THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                                  &m_blit_bloom_1d_horizontal_pipeline,
                                                  blit_vertex_shader,
                                                  blit_bloom_1d_horizontal_pixel_shader,
                                                  &m_pipeline_layout,
                                                  "Blit bloom 1D horizontal pipeline"));
        THROW_IF_FALSE(m_context->create_pipeline(blit_pipeline_desc,
                                                  &m_blit_bloom_1d_vertical_pipeline,
                                                  blit_vertex_shader,
                                                  blit_bloom_1d_vertical_pixel_shader,
                                                  &m_pipeline_layout,
                                                  "Blit bloom 1D vertical pipeline"));
    }

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

    const unsigned frame_slot = m_context->get_frame_slot(m_frames_in_flight);
    THROW_IF_FALSE(m_context->acquire_swapchain_image());

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
                m_camera_transition_start =
                    qhenki::math::Transform::lerp(m_camera_transition_start, current.hierarchy.world_transform, blend);
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

    const float scale = 2.f + (1.f + std::sin(-time_sec * 2.f)) * 0.5f * 1.5f;
    const XMMATRIX stencil_world_mat =
        XMMatrixMultiply(XMMatrixMultiply(XMMatrixIdentity(), XMMatrixScaling(scale, 1.f, scale)),
                         XMMatrixTranslation(m_cube_parent.world_transform.translation.x,
                                             0.0f,
                                             m_cube_parent.world_transform.translation.z));

    const PerspectiveCamera& active_camera = m_active_camera_index == 0 ? m_camera
                                           : m_active_camera_index == 1 ? m_cube_camera
                                                                        : m_orbit_camera;

    qhenki::math::Transform display_world;
    if (m_camera_transition_t >= 0.f)
    {
        const float blend = std::min(m_camera_transition_t, 1.f);
        display_world =
            qhenki::math::Transform::lerp(m_camera_transition_start, active_camera.hierarchy.world_transform, blend);
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
    XMStoreFloat4x4(&cb.stencil_world, XMMatrixTranspose(stencil_world_mat));
    cb.camera_position = display_world.translation;
    cb.time = time_sec;

    const auto buffer_pointer = m_context->map_buffer(m_matrix_buffers[frame_slot]);
    assert(buffer_pointer);
    memcpy(buffer_pointer, &cb, sizeof(FrameConstants));
    m_context->unmap_buffer(m_matrix_buffers[frame_slot]);

    THROW_IF_FALSE(m_context->reset_command_pool(&m_cmd_pools[frame_slot]));

    THROW_IF_FALSE(m_context->reset_command_list(&m_cmd_lists[frame_slot], m_cmd_pools[frame_slot]));
    auto& cmd_list = m_cmd_lists[frame_slot];

    qhenki::gfx::RenderTarget color{
        .clear_params = {.clear_color_value = {1.f, 1.f, 1.f, 0.f}},
        .clear_type = qhenki::gfx::RenderTarget::COLOR,
        .texture = &m_offscreen_texture.tex,
    };
    qhenki::gfx::RenderTarget depth{
        .clear_params = {.dsv_clear_params = {1.f, 0}},
        .clear_type = static_cast<qhenki::gfx::RenderTarget::ClearType>(qhenki::gfx::RenderTarget::DEPTH |
                                                                        qhenki::gfx::RenderTarget::STENCIL),
        .texture = &m_depth_buffer,
    };
    m_context->start_render_pass(&cmd_list, 1, &color, &depth);

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

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_skybox_pipeline));
    size_t gpu_descriptor_heap_index = 0;

    if (m_context->is_compatibility())
    {
        std::array buffer = {&m_matrix_buffers[frame_slot]};
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
        const size_t cbv_off = gpu_descriptor_heap_index++;
        const qhenki::gfx::Descriptor cbv_descriptor{.heap = &m_GPU_heap, .offset = cbv_off};
        THROW_IF_FALSE(m_context->copy_descriptors(1, m_matrix_descriptors[frame_slot], cbv_descriptor));
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 0, cbv_descriptor));

        const size_t srv1_b0 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_skybox_texture_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = srv1_b0}));
        const size_t srv1_b1 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_skybox_texture_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = srv1_b1}));
        THROW_IF_FALSE(m_context->set_descriptor_table(
            &cmd_list, m_pipeline_layout, 1, qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = srv1_b0}));

        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 2, m_sampler_descriptor));
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
    auto vb_offset = [](const Mesh::AccessorBufferView& abv)
    {
        return abv.buffer_view.offset + abv.accessor.offset;
    };
    auto vb_length = [](const Mesh::AccessorBufferView& abv)
    {
        return abv.buffer_view.length;
    };
    auto vb_stride = [&stride_from_accessor](const Mesh::AccessorBufferView& abv)
    {
        return abv.buffer_view.stride != 0 ? abv.buffer_view.stride
                                           : stride_from_accessor(abv.accessor.component_type, abv.accessor.type);
    };
    const std::array skybox_vbs = {&m_skybox_buffer};
    const std::array skybox_vb_offsets = {vb_offset(m_skybox_mesh.position)};
    const std::array skybox_vb_lengths = {vb_length(m_skybox_mesh.position)};
    const std::array skybox_vb_strides = {vb_stride(m_skybox_mesh.position)};
    m_context->bind_vertex_buffers(&cmd_list,
                                   0,
                                   skybox_vbs.size(),
                                   skybox_vbs.data(),
                                   skybox_vb_lengths.data(),
                                   skybox_vb_strides.data(),
                                   skybox_vb_offsets.data());
    const unsigned index_offset = static_cast<unsigned>(m_skybox_mesh.index.buffer_view.offset +
                                                        m_skybox_mesh.index.accessor.offset);
    const auto index_type = m_skybox_mesh.index.accessor.component_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                              ? qhenki::gfx::IndexType::UINT16
                              : qhenki::gfx::IndexType::UINT32;
    m_context->bind_index_buffer(&cmd_list, m_skybox_buffer, index_type, index_offset);
    m_context->draw_indexed(&cmd_list, static_cast<unsigned>(m_skybox_mesh.index.accessor.count), 1, 0, 0, 0);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_cube_pipeline));
    m_context->draw(&cmd_list, 36u, 0);

    const std::array bevel_vbs = {&m_bevel_cube_mesh.buffer, &m_bevel_cube_mesh.buffer};
    const std::array bevel_vb_offsets = {vb_offset(m_bevel_cube_mesh.position), vb_offset(m_bevel_cube_mesh.normal)};
    const std::array bevel_vb_lengths = {vb_length(m_bevel_cube_mesh.position), vb_length(m_bevel_cube_mesh.normal)};
    const std::array bevel_vb_strides = {vb_stride(m_bevel_cube_mesh.position), vb_stride(m_bevel_cube_mesh.normal)};
    const unsigned bevel_index_offset = static_cast<unsigned>(m_bevel_cube_mesh.index.buffer_view.offset +
                                                              m_bevel_cube_mesh.index.accessor.offset);
    const auto bevel_index_type = m_bevel_cube_mesh.index.accessor.component_type ==
                                          TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                                    ? qhenki::gfx::IndexType::UINT16
                                    : qhenki::gfx::IndexType::UINT32;

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_stencil_pipeline));
    const std::array stencil_vbs = {&m_stencil_mesh.buffer};
    const std::array stencil_vb_offsets = {vb_offset(m_stencil_mesh.position)};
    const std::array stencil_vb_lengths = {vb_length(m_stencil_mesh.position)};
    const std::array stencil_vb_strides = {vb_stride(m_stencil_mesh.position)};
    m_context->bind_vertex_buffers(&cmd_list,
                                   0,
                                   stencil_vbs.size(),
                                   stencil_vbs.data(),
                                   stencil_vb_lengths.data(),
                                   stencil_vb_strides.data(),
                                   stencil_vb_offsets.data());
    const unsigned stencil_index_offset = static_cast<unsigned>(m_stencil_mesh.index.buffer_view.offset +
                                                                m_stencil_mesh.index.accessor.offset);
    const auto stencil_index_type = m_stencil_mesh.index.accessor.component_type ==
                                            TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
                                      ? qhenki::gfx::IndexType::UINT16
                                      : qhenki::gfx::IndexType::UINT32;
    m_context->bind_index_buffer(&cmd_list, m_stencil_mesh.buffer, stencil_index_type, stencil_index_offset);
    m_context->draw_indexed(&cmd_list, static_cast<unsigned>(m_stencil_mesh.index.accessor.count), 1, 0, 0, 0);

    THROW_IF_FALSE(m_context->bind_pipeline(&cmd_list, m_bevel_cube_pipeline));
    m_context->bind_vertex_buffers(&cmd_list,
                                   0,
                                   bevel_vbs.size(),
                                   bevel_vbs.data(),
                                   bevel_vb_lengths.data(),
                                   bevel_vb_strides.data(),
                                   bevel_vb_offsets.data());
    m_context->bind_index_buffer(&cmd_list, m_bevel_cube_mesh.buffer, bevel_index_type, bevel_index_offset);
    constexpr unsigned bevel_instance_count = grid_size * grid_size;
    m_context->draw_indexed(
        &cmd_list, static_cast<unsigned>(m_bevel_cube_mesh.index.accessor.count), bevel_instance_count, 0, 0, 0);

    m_context->end_render_pass(&cmd_list);

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
        .texture = &m_bloom_textures[m_starting_bloom_index].tex,
    };
    m_context->start_render_pass(&cmd_list, 1, &blit_target, nullptr);
    const unsigned bloom_w = m_bloom_textures.front().tex.desc.width;
    const unsigned bloom_h = m_bloom_textures.front().tex.desc.height;
    const qhenki::gfx::Viewport bloom_viewport{
        .top_left_x = 0,
        .top_left_y = 0,
        .width = static_cast<float>(bloom_w),
        .height = static_cast<float>(bloom_h),
        .min_depth = 0.0f,
        .max_depth = 1.0f,
    };
    const qhenki::gfx::Rect bloom_scissor{
        .left = 0,
        .top = 0,
        .width = bloom_w,
        .height = bloom_h,
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
        const size_t lum_b0 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_offscreen_texture.srv_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = lum_b0}));
        const size_t lum_b1 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_offscreen_texture.srv_descriptor,
                                                   qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = lum_b1}));
        THROW_IF_FALSE(m_context->set_descriptor_table(
            &cmd_list, m_pipeline_layout, 1, qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = lum_b0}));
    }

    m_context->draw(&cmd_list, 3, 0);

    m_context->end_render_pass(&cmd_list);

    // Start blur passes
    std::array<size_t, BLOOM_TEXTURE_COUNT> blur_srv_start{};
    if (!m_context->is_compatibility())
    {
        for (unsigned i = 0; i < m_bloom_textures.size(); i++)
        {
            const size_t bb0 = gpu_descriptor_heap_index++;
            blur_srv_start[i] = bb0;
            THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                       m_bloom_textures[i].srv_descriptor,
                                                       qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = bb0}));
            const size_t bb1 = gpu_descriptor_heap_index++;
            THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                       m_bloom_textures[i].srv_descriptor,
                                                       qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = bb1}));
        }
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
        m_context->set_barrier_resource(1, &swap_barriers[2], m_swapchain);

        m_starting_bloom_index = 1 - m_starting_bloom_index;

        if (i == iterations)
        {
            m_context->issue_barrier(&cmd_list, 3, swap_barriers.data());
            break;
        }
        m_context->issue_barrier(&cmd_list, 2, swap_barriers.data());

        qhenki::gfx::RenderTarget blit_target_blur{
            .clear_params = {.clear_color_value = {0.f, 0.f, 0.f, 0.f}},
            .clear_type = qhenki::gfx::RenderTarget::COLOR,
            .texture = &bloom1.tex,
        };
        m_context->start_render_pass(&cmd_list, 1, &blit_target_blur, nullptr);
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
            THROW_IF_FALSE(m_context->set_descriptor_table(
                &cmd_list,
                m_pipeline_layout,
                1,
                qhenki::gfx::Descriptor{.heap = &m_GPU_heap, .offset = blur_srv_start[1 - m_starting_bloom_index]}));
        }

        m_context->draw(&cmd_list, 3, 0);

        m_context->end_render_pass(&cmd_list);
    }

    // Composite image into swapchain backbuffer
    std::array clear_values = {0.f, 0.f, 0.f, 1.f};
    m_context->start_render_pass(&cmd_list, clear_values.data(), nullptr);
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
        const size_t composite_b0 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   m_offscreen_texture.srv_descriptor,
                                                   qhenki::gfx::Descriptor{
                                                       .heap = &m_GPU_heap,
                                                       .offset = composite_b0,
                                                   }));
        const size_t composite_b1 = gpu_descriptor_heap_index++;
        THROW_IF_FALSE(m_context->copy_descriptors(1,
                                                   final_bloom.srv_descriptor,
                                                   qhenki::gfx::Descriptor{
                                                       .heap = &m_GPU_heap,
                                                       .offset = composite_b1,
                                                   }));
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list,
                                                       m_pipeline_layout,
                                                       1,
                                                       qhenki::gfx::Descriptor{
                                                           .heap = &m_GPU_heap,
                                                           .offset = composite_b0,
                                                       }));
        THROW_IF_FALSE(m_context->set_descriptor_table(&cmd_list, m_pipeline_layout, 2, m_sampler_descriptor));
    }

    m_context->draw(&cmd_list, 3, 0);

    ImGui::Render();
    m_context->render_imgui_draw_data(&cmd_list);

    m_context->end_render_pass(&cmd_list);

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
    m_context->set_barrier_resource(1, &end_barriers[0], m_swapchain);
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

void RetroExampleApp::resize(const unsigned width, const unsigned height)
{
    m_context->wait_idle(qhenki::gfx::QueueType::GRAPHICS);
    const qhenki::gfx::TextureDesc depth_desc{
        .width = width,
        .height = height,
        .format = m_depth_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::DEPTH_STENCIL_WRITE,
        .usage = qhenki::gfx::TextureDesc::DEPTH_STENCIL,
        .clear_depth_value = {.depth = 1.f, .stencil = 0},
    };
    THROW_IF_FALSE(m_context->create_texture(depth_desc, &m_depth_buffer, "Depth Buffer Texture"));

    const qhenki::gfx::TextureDesc offscreen_rt_desc{
        .width = width,
        .height = height,
        .format = m_offscreen_rt_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .usage = qhenki::gfx::TextureDesc::RENDER_TARGET | qhenki::gfx::TextureDesc::SHADER_RESOURCE,
        .clear_color_value = {1.f, 1.f, 1.f, 0.f},
    };
    THROW_IF_FALSE(m_context->create_texture(offscreen_rt_desc, &m_offscreen_texture.tex, "Offscreen RT"));
    m_offscreen_texture.srv_descriptor.offset = m_frames_in_flight + 1;
    THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_offscreen_texture.tex,
                                                            &m_CPU_heap,
                                                            &m_offscreen_texture.srv_descriptor));

    qhenki::gfx::TextureDesc blit_rt_desc{
        .width = width / 4,
        .height = height / 4,
        .format = m_offscreen_rt_format,
        .dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
        .initial_layout = qhenki::gfx::Layout::RENDER_TARGET,
        .usage = qhenki::gfx::TextureDesc::RENDER_TARGET | qhenki::gfx::TextureDesc::SHADER_RESOURCE,
        .clear_color_value = {0.f, 0.f, 0.f, 0.f},
    };
    for (unsigned i = 0; i < m_bloom_textures.size(); i++)
    {
        blit_rt_desc.initial_layout = i == m_starting_bloom_index ? qhenki::gfx::Layout::RENDER_TARGET
                                                                  : qhenki::gfx::Layout::SHADER_RESOURCE;
        THROW_IF_FALSE(m_context->create_texture(blit_rt_desc,
                                                 &m_bloom_textures[i].tex,
                                                 qhenki::util::format_string("Bloom RT %u", i).buffer.data()));
        m_bloom_textures[i].srv_descriptor.offset = m_frames_in_flight + 2 + i;
        THROW_IF_FALSE(m_context->create_descriptor_shader_view(m_bloom_textures[i].tex,
                                                                &m_CPU_heap,
                                                                &m_bloom_textures[i].srv_descriptor));
    }
}

void RetroExampleApp::destroy()
{
    m_context->destroy_imgui();
}

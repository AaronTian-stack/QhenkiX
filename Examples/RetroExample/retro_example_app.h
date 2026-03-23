#pragma once

#include <mutex>
#include "scene.h"
#include "shared_structs.h"

#include "qhenki/application.h"

constexpr auto BLOOM_TEXTURE_COUNT = 2u;

class RetroExampleApp : public qhenki::Application
{
    struct Mesh
    {
        struct Accessor
        {
            size_t offset = 0;
            size_t count = 0;
            int type = -1;           // scalar, vector...
            int component_type = -1; // int, byte, short, float...
        };
        struct BufferView
        {
            size_t offset = 0;
            size_t length = 0;
            size_t stride = 0;
        };
        using AccessorBufferView = std::pair<Accessor, BufferView>;
        AccessorBufferView position;
        AccessorBufferView normal;
        AccessorBufferView texcoord;
        AccessorBufferView index;
        qhenki::gfx::Buffer buffer{};
    };

    qhenki::gfx::PipelineLayout m_pipeline_layout{};

    qhenki::gfx::Buffer m_skybox_buffer{};

    qhenki::gfx::GraphicsPipeline m_skybox_pipeline{};

    qhenki::gfx::GraphicsPipeline m_cube_pipeline{};

    qhenki::gfx::GraphicsPipeline m_bevel_cube_pipeline{};

    qhenki::gfx::GraphicsPipeline m_stencil_pipeline{};

    qhenki::gfx::GraphicsPipeline m_blit_copy_pipeline{};
    qhenki::gfx::GraphicsPipeline m_blit_luminance_pipeline{};
    qhenki::gfx::GraphicsPipeline m_blit_bloom_1d_horizontal_pipeline{};
    qhenki::gfx::GraphicsPipeline m_blit_bloom_1d_vertical_pipeline{};

    std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools{};
    std::array<qhenki::gfx::CommandList, m_frames_in_flight> m_cmd_lists{};

    Mesh m_skybox_mesh;

    Mesh m_stencil_mesh;

    Mesh m_bevel_cube_mesh;

    std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
    std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

    qhenki::gfx::Descriptor m_skybox_texture_descriptor{};
    qhenki::gfx::Texture m_skybox_texture{};
    qhenki::gfx::Descriptor m_sampler_descriptor{};
    qhenki::gfx::Descriptor m_sampler_linear_descriptor{};

    qhenki::gfx::Texture m_depth_buffer{};
    qhenki::gfx::Descriptor m_depth_buffer_descriptor{};

    struct TextureDescriptor
    {
        qhenki::gfx::Texture tex;
        qhenki::gfx::Descriptor srv_descriptor;
    };

    TextureDescriptor m_offscreen_texture{};
    std::array<TextureDescriptor, BLOOM_TEXTURE_COUNT> m_bloom_textures{};
    unsigned m_starting_bloom_index = 0;

    qhenki::gfx::DescriptorHeap m_CPU_heap{};
    qhenki::gfx::DescriptorHeap m_GPU_heap{};
    qhenki::gfx::DescriptorHeap m_dsv_heap{};
    qhenki::gfx::DescriptorHeap m_sampler_heap{};

    SceneObject m_camera_target{};
    PerspectiveCamera m_camera{};
    float m_target_distance = 2.0f;

    SceneObject m_cube_parent{};
    SceneObject m_cube_child{};
    PerspectiveCamera m_cube_camera{};

    PerspectiveCamera m_orbit_camera{};

    unsigned m_active_camera_index = 1;

    static const DXGI_FORMAT m_offscreen_rt_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static const DXGI_FORMAT m_depth_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

protected:
    void init_display_window(void* payload) override;
    void create() override;
    void render() override;
    void resize(unsigned width, unsigned height) override;
    void destroy() override;

public:
    struct Payload
    {
        bool fullscreen;
    };
};

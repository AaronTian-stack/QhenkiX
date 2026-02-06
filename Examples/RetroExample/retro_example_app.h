#pragma once

#include <mutex>
#include "scene.h"

#include "qhenki/application.h"

class RetroExampleApp : public qhenki::Application
{
    qhenki::gfx::PipelineLayout m_pipeline_layout{};
    qhenki::gfx::GraphicsPipeline m_pipeline{};
    qhenki::gfx::Shader m_vertex_shader{};
    qhenki::gfx::Shader m_pixel_shader{};

    std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools{};
    std::array<qhenki::gfx::CommandList, m_frames_in_flight> m_cmd_lists{};

    qhenki::gfx::Buffer m_vertex_buffer{};
    qhenki::gfx::Buffer m_index_buffer{};

    std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
    std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

    qhenki::gfx::Descriptor m_texture_descriptor{};
    qhenki::gfx::Texture m_texture{};
    qhenki::gfx::Descriptor m_sampler_descriptor{};

    qhenki::gfx::Texture m_depth_buffer{};
    qhenki::gfx::Descriptor m_depth_buffer_descriptor{};

    qhenki::gfx::DescriptorHeap m_CPU_heap{};
    qhenki::gfx::DescriptorHeap m_GPU_heap{};
    qhenki::gfx::DescriptorHeap m_dsv_heap{};
    qhenki::gfx::DescriptorHeap m_sampler_heap{};

    SceneObject m_camera_target{};
    PerspectiveCamera m_camera{};
    float m_target_distance = 2.0f;

protected:
    void init_display_window(void* payload) override;
    void create() override;
    void render() override;
    void resize(int width, int height) override;
    void destroy() override;

public:
    struct Payload
    {
        bool fullscreen;
    };
};

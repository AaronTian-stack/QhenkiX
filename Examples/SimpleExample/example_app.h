#pragma once

#include "qhenki/application.h"
#include "shared_structs.h"

class ExampleApp : public qhenki::Application
{
    qhenki::gfx::PipelineLayout m_pipeline_layout{};
    qhenki::gfx::GraphicsPipeline m_pipeline{};
    qhenki::gfx::Shader m_vertex_shader{};
    qhenki::gfx::Shader m_pixel_shader{};

    // One Command Pool per frame, per thread. Pool allocates lists
    // Command pools for main thread
    std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools{};
    std::array<qhenki::gfx::CommandList, m_frames_in_flight> m_cmd_lists{};

    qhenki::gfx::Buffer m_vertex_buffer{};
    qhenki::gfx::Buffer m_index_buffer{};

    std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
    std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

    qhenki::gfx::Descriptor m_texture_descriptor{};
    qhenki::gfx::Texture m_texture{};
    qhenki::gfx::Descriptor m_sampler_descriptor{};

    qhenki::gfx::DescriptorHeap m_CPU_heap{};
    qhenki::gfx::DescriptorHeap m_GPU_heap{};

    qhenki::gfx::DescriptorHeap m_sampler_heap{};

    CameraMatrices m_matrices{};

protected:
    void create() override;
    void render() override;
    void resize(unsigned width, unsigned height) override;
    void destroy() override;
};

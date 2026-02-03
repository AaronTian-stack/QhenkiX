#pragma once

#include <tsl/robin_map.h>
#include <mutex>
#include "gltf_loader.h"

#include "qhenki/application.h"

class gltfViewerApp : public qhenki::Application
{
    qhenki::gfx::PipelineLayout m_pipeline_layout{};
    qhenki::gfx::GraphicsPipeline m_pipeline{};
    qhenki::gfx::Shader m_vertex_shader{};
    qhenki::gfx::Shader m_pixel_shader{};

    // One Command Pool per frame, per thread. Pool allocates lists
    // Command pools for main thread
    std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools{};
    std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_thread{};
    std::array<qhenki::gfx::CommandList, m_frames_in_flight> m_cmd_lists{};

    std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
    std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

    qhenki::gfx::Descriptor m_model_descriptor{}; // Model matrix descriptor (compatibility only)
    qhenki::gfx::Buffer m_model_buffer{};         // Model matrix (compatibility only)

    qhenki::gfx::Texture m_depth_buffer{};
    qhenki::gfx::Descriptor m_depth_buffer_descriptor{};

    qhenki::gfx::DescriptorHeap m_CPU_heap{};
    qhenki::gfx::DescriptorHeap m_GPU_heap{};

    qhenki::gfx::Descriptor m_model_material_descriptor{};
    std::vector<qhenki::gfx::Descriptor> m_model_texture_descriptors{};

    qhenki::gfx::Descriptor m_model_gltfTexture_descriptor{};

    qhenki::gfx::DescriptorHeap m_dsv_heap{};

    qhenki::gfx::DescriptorHeap m_sampler_heap{};
    std::vector<qhenki::gfx::Descriptor> m_sampler_descriptors{};

    std::mutex m_model_mutex;
    std::atomic_int m_model_index_to_load_into = 0;
    std::array<GLTFModel, m_frames_in_flight> m_models{};
    tsl::robin_map<std::string, int> m_attribute_to_slot{
        {"POSITION", 0}, {"NORMAL", 1}, {"COLOR_0", 2}, {"TEXCOORD_0", 3},
        //{"TEXCOORD_1", 4},
    };

    bool m_tearing = false;

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

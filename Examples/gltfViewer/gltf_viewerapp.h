#pragma once

#include <application.h>

#include "graphics/arcball_controller.h"
#include "graphics/perspective_camera.h"

#include "gltf_loader.h"
#include <tsl/robin_map.h>
#include <mutex>

class gltfViewerApp : public qhenki::Application
{
	qhenki::gfx::PipelineLayout m_pipeline_layout_{};
	qhenki::gfx::GraphicsPipeline m_pipeline_{};
	qhenki::gfx::Shader m_vertex_shader_{};
	qhenki::gfx::Shader m_pixel_shader_{};

	// One Command Pool per frame, per thread. Pool allocates lists
	// Command pools for main thread
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_{};
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_thread_{};

	std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors_{};
	std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers_{};

	qhenki::gfx::Buffer m_model_buffer{};

	qhenki::gfx::Descriptor m_sampler_descriptor_{};
	qhenki::gfx::Sampler m_sampler_{};

	qhenki::gfx::Texture m_depth_buffer_{};
	qhenki::gfx::Descriptor m_depth_buffer_descriptor_{};

	qhenki::gfx::DescriptorHeap m_CPU_heap_{};
	qhenki::gfx::DescriptorHeap m_GPU_heap_{};

	qhenki::gfx::DescriptorHeap m_dsv_heap_{};

	qhenki::gfx::DescriptorHeap m_sampler_heap_{};

	qhenki::PerspectiveCamera m_camera_{};
	qhenki::ArcBallController m_camera_controller_{};

	std::mutex m_model_mutex_;
	GLTFModel m_model_{};
	tsl::robin_map<std::string, int> attribute_to_slot
	{
		{"POSITION", 0},
		{"NORMAL", 1},
		{"COLOR_0", 2},
		{"TEXCOORD_0", 3},
		//{"TEXCOORD_1", 4},
	};

	void update_global_transform(GLTFModel& model, GLTFModel::Node& node);

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~gltfViewerApp() override;
};


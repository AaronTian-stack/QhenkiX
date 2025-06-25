#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "gltf_loader.h"
#include <tsl/robin_map.h>
#include <mutex>

#include "qhenkiX/application.h"
#include "qhenkiX/arcball_controller.h"
#include "qhenkiX/perspective_camera.h"

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

	std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
	std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

	qhenki::gfx::Descriptor m_model_matrix_descriptor{};
	qhenki::gfx::Buffer m_model_buffer{};

	qhenki::gfx::Texture m_depth_buffer{};
	qhenki::gfx::Descriptor m_depth_buffer_descriptor{};

	qhenki::gfx::DescriptorHeap m_CPU_heap{};
	qhenki::gfx::DescriptorHeap m_GPU_heap{};

	std::vector<qhenki::gfx::Descriptor> m_model_material_descriptors{};
	std::vector<qhenki::gfx::Descriptor> m_model_texture_descriptors{};

	qhenki::gfx::DescriptorHeap m_dsv_heap{};

	qhenki::gfx::DescriptorHeap m_sampler_heap{};
	std::vector<qhenki::gfx::Descriptor> m_sampler_descriptors{};

	qhenki::PerspectiveCamera m_camera{};
	qhenki::ArcBallController m_camera_controller{};

	std::mutex m_model_mutex;
	GLTFModel m_model{};
	tsl::robin_map<std::string, int> m_attribute_to_slot
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


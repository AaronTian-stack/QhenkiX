#pragma once

#include <application.h>

struct CameraMatrices
{
	XMFLOAT4X4 viewProj;
	XMFLOAT4X4 invViewProj;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 color;
	XMFLOAT2 texcoord;
};

class ImGUIExampleApp : public qhenki::Application
{
	qhenki::gfx::PipelineLayout m_pipeline_layout_{};
	qhenki::gfx::GraphicsPipeline m_pipeline_{};
	qhenki::gfx::Shader m_vertex_shader_{};
	qhenki::gfx::Shader m_pixel_shader_{};

	// One Command Pool per frame, per thread. Pool allocates lists
	// Command pools for main thread
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_{};

	qhenki::gfx::Buffer m_vertex_buffer_{};
	qhenki::gfx::Buffer m_index_buffer_{};

	std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors_{};
	std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers_{};

	qhenki::gfx::Descriptor m_texture_descriptor_{};
	qhenki::gfx::Texture m_texture_{};
	qhenki::gfx::Descriptor m_sampler_descriptor_{};
	qhenki::gfx::Sampler m_sampler_{};

	qhenki::gfx::DescriptorHeap m_CPU_heap_{};
	qhenki::gfx::DescriptorHeap m_GPU_heap_{};

	qhenki::gfx::DescriptorHeap m_sampler_heap_{};

	CameraMatrices matrices_{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ImGUIExampleApp() override;
};


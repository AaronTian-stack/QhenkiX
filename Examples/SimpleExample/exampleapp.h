#pragma once
#include "qhenkiX/application.h"

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

class ExampleApp : public qhenki::Application
{
	qhenki::gfx::PipelineLayout m_pipeline_layout{};
	qhenki::gfx::GraphicsPipeline m_pipeline{};
	qhenki::gfx::Shader m_vertex_shader{};
	qhenki::gfx::Shader m_pixel_shader{};

	// One Command Pool per frame, per thread. Pool allocates lists
	// Command pools for main thread
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools{};

	qhenki::gfx::Buffer m_vertex_buffer{};
	qhenki::gfx::Buffer m_index_buffer{};

	std::array<qhenki::gfx::Descriptor, m_frames_in_flight> m_matrix_descriptors{};
	std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers{};

	qhenki::gfx::Descriptor m_texture_descriptor{};
	qhenki::gfx::Texture m_texture{};
	qhenki::gfx::Descriptor m_sampler_descriptor{};
	qhenki::gfx::Sampler m_sampler{};

	qhenki::gfx::DescriptorHeap m_CPU_heap{};
	qhenki::gfx::DescriptorHeap m_GPU_heap{};

	qhenki::gfx::DescriptorHeap m_sampler_heap{};

	CameraMatrices m_matrices{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ExampleApp() override;
};


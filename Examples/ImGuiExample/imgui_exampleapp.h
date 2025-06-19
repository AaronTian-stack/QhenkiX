#pragma once

#include <application.h>

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 color;
};

class ImGUIExampleApp : public qhenki::Application
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

	qhenki::gfx::DescriptorHeap m_CPU_heap{};
	qhenki::gfx::DescriptorHeap m_GPU_heap{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ImGUIExampleApp() override;
};


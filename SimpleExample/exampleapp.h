#pragma once

#include <application.h>

struct CameraMatrices
{
	XMFLOAT4X4 viewProj;
	XMFLOAT4X4 invViewProj;
};

class ExampleApp : public Application
{
	qhenki::gfx::GraphicsPipeline m_pipeline_{};
	qhenki::gfx::Shader m_vertex_shader_{};
	qhenki::gfx::Shader m_pixel_shader_{};

	// One Command Pool per frame, per thread. Pool allocates lists
	// Command pools for main thread
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_{};

	qhenki::gfx::Buffer m_vertex_buffer_{};
	qhenki::gfx::Buffer m_index_buffer_{};
	std::array<qhenki::gfx::Buffer, m_frames_in_flight> m_matrix_buffers_{};

	qhenki::gfx::Fence m_fence_frame_ready_{};
	std::array<uint64_t, m_frames_in_flight> m_fence_frame_ready_val_{};

	CameraMatrices matrices_{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ExampleApp() override;
};


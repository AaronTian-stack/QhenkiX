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

	// TODO: one commandpool per frame, per thread. pool allocates lists
	// command pools for main thread
	std::array<qhenki::gfx::CommandPool, m_frames_in_flight> m_cmd_pools_{};

	qhenki::gfx::Buffer m_vertex_buffer_{};
	qhenki::gfx::Buffer m_index_buffer_{};
	qhenki::gfx::Buffer matrix_buffer_{};

	CameraMatrices matrices_{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ExampleApp() override;
};


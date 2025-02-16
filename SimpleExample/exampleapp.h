#pragma once

#include <application.h>

struct CameraMatrices
{
	XMFLOAT4X4 viewProj;
	XMFLOAT4X4 invViewProj;
};

/**
* @class ExampleApp
* @brief Renders a spinning cube.
*
* Sets up the necessary graphics pipeline, shaders, command lists, and buffers to render a spinning cube.
* It overrides the create, render, resize, and destroy methods to manage the lifecycle of the application.
*/
class ExampleApp : public Application
{
	qhenki::graphics::GraphicsPipeline m_pipeline_{};
	qhenki::graphics::Shader m_vertex_shader_{};
	qhenki::graphics::Shader m_pixel_shader_{};

	// TODO: one commandpool per frame, per thread. pool allocates lists
	// command pools for main thread
	std::array<qhenki::graphics::CommandPool, m_frames_in_flight> m_cmd_pools_{};

	qhenki::graphics::Buffer m_vertex_buffer_{};
	qhenki::graphics::Buffer m_index_buffer_{};
	qhenki::graphics::Buffer matrix_buffer_{};

	CameraMatrices matrices_{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;

public:
	~ExampleApp() override;
};


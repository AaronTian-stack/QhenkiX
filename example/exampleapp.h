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
	qhenki::GraphicsPipeline pipeline{};
	qhenki::Shader vertex_shader{};
	qhenki::Shader pixel_shader{};

	// TODO: one commandpool per frame, per thread. pool allocates lists

	//qhenki::CommandPool cmd_pool{};
	qhenki::CommandList cmd_list{};

	qhenki::Buffer vertex_buffer{};
	qhenki::Buffer index_buffer{};
	qhenki::Buffer matrix_buffer{};

	CameraMatrices matrices{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;
};


#pragma once

#include <application.h>

class ExampleApp : public Application
{
	qhenki::GraphicsPipeline pipeline{};
	qhenki::Shader vertex_shader{};
	qhenki::Shader pixel_shader{};

	// TODO: one commandpool per frame, per thread. pool allocates lists

	//qhenki::CommandPool cmd_pool{};
	qhenki::CommandList cmd_list{};

	qhenki::Buffer vertex_buffer{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;
};


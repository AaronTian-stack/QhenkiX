#pragma once

#include <application.h>

class ExampleApp : public Application
{
	vendetta::GraphicsPipeline pipeline{};
	vendetta::Shader vertex_shader{};
	vendetta::Shader pixel_shader{};

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;
};


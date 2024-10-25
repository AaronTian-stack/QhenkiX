#pragma once

#include <application.h>

class ExampleApp : public Application
{
	ComPtr<ID3D11VertexShader> vertex_shader;
	ComPtr<ID3D11PixelShader> pixel_shader;

protected:
	void create() override;
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;
};


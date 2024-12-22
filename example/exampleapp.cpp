#include "exampleapp.h"

void ExampleApp::create()
{
	// create shaders
	context_->create_shader(vertex_shader, L"base-shaders/BaseShader.vs.hlsl", vendetta::ShaderType::VERTEX, {});
	context_->create_shader(pixel_shader, L"base-shaders/BaseShader.ps.hlsl", vendetta::ShaderType::PIXEL, {});
	// create pipeline
	
}

void ExampleApp::render()
{
	// clear back buffer
	// set viewport
	// bind pipeline
	// TODO: bind buffer
	// draw
	// present

	context_->present(swapchain_);
}

void ExampleApp::resize(int width, int height)
{
	
}

void ExampleApp::destroy()
{
	// destroy pipeline then shaders
}

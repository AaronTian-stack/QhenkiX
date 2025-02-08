#include "viewport.h"

void Viewport::apply(D3D11Context& context, bool center_camera)
{
	D3D11_VIEWPORT viewport =
	{
		.TopLeftX = screen_X_,
		.TopLeftY = screen_Y_,
		.Width = screen_width_,
		.Height = screen_height_,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	//context.device_context->RSSetViewports(
	//	1,
	//	&viewport);
	camera->viewport_width = screen_width_;
	camera->viewport_height = screen_height_;
	if (center_camera)
		camera->position_ = { world_width_ / 2, world_height_ / 2, 0 };
	camera->update();
}

void Viewport::update(D3D11Context& context, int screen_width, int screen_height, bool center_camera)
{
	apply(context, center_camera);
}

void Viewport::unproject(XMFLOAT2& screen_coordinate)
{
	XMFLOAT3 screen = { screen_coordinate.x, screen_coordinate.y, 0 };
	camera->unproject(screen, screen_X_, screen_Y_, screen_width_, screen_height_);
	screen_coordinate = { screen.x, screen.y };
}

void Viewport::project(XMFLOAT2& world_coordinate)
{
	XMFLOAT3 world = { world_coordinate.x, world_coordinate.y, 0 };
	camera->project(world, screen_X_, screen_Y_, screen_width_, screen_height_);
	world_coordinate = { world.x, world.y };
}

void Viewport::unproject(XMFLOAT3& screen_coordinate)
{
	camera->unproject(screen_coordinate, screen_X_, screen_Y_, screen_width_, screen_height_);
}

void Viewport::project(XMFLOAT3& world_coordinate)
{
	camera->project(world_coordinate, screen_X_, screen_Y_, screen_width_, screen_height_);
}

void Viewport::set_screen_bounds(float x, float y, float width, float height)
{
	screen_X_ = x;
	screen_Y_ = y;
	screen_width_ = width;
	screen_height_ = height;
}

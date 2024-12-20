#pragma once
#include <DirectXMath.h>

#include "graphics/camera.h"
#include "graphics/d3d11/d3d11_context.h"

using namespace DirectX;

class Viewport
{
protected:
	Camera* camera;
	float world_width_, world_height_;
	float screen_X_, screen_Y_, screen_width_, screen_height_;
	Viewport(float world_width, float world_height, Camera* camera) : camera(camera), world_width_(world_width),
	                                                                  world_height_(world_height), screen_X_(0),
	                                                                  screen_Y_(0),
	                                                                  screen_width_(0),
	                                                                  screen_height_(0)
	{}

public:
	void apply(D3D11Context& context, bool center_camera);
	virtual void update(D3D11Context& context, int screen_width, int screen_height, bool center_camera = false);

	void unproject(XMFLOAT2& screen_coordinate);
	void project(XMFLOAT2& world_coordinate);

	void unproject(XMFLOAT3& screen_coordinate);
	void project(XMFLOAT3& world_coordinate);

	// TODO: scissors

	void set_screen_bounds(float x, float y, float width, float height);

	int get_left_gutter_width() const { return screen_X_; }
	int get_right_gutter_x() const { return screen_X_ + screen_width_; }
	int get_right_gutter_width() const { assert(false); } // TODO
	int get_bottom_gutter_height() const { return screen_Y_; }
	int get_top_gutter_y() const { return screen_Y_ + screen_height_; }
	int get_top_gutter_height() const { assert(false); } // TODO
};

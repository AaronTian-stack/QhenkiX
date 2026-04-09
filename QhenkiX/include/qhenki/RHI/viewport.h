#pragma once

namespace qhenki::gfx
{
struct Viewport
{
    float top_left_x;
    float top_left_y;
    float width;
    float height;
    float min_depth;
    float max_depth;
};

struct Rect
{
    long left;
    long top;
    long front;
    long right;
    long bottom;
    long back;
};
} // namespace qhenki::gfx

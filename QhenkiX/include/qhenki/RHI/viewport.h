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
    int32_t left;
    int32_t top;
    uint32_t width;
    uint32_t height;
};
} // namespace qhenki::gfx

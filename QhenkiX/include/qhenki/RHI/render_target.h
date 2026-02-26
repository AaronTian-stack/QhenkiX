#pragma once

#include <array>

#include "qhenki/utility/math_util.h"
#include "texture.h"

namespace qhenki::gfx
{
struct RenderTarget
{
    union ClearParams
    {
        std::array<float, 4> clear_color_value = {0.0f, 0.0f, 0.0f, 1.0f};
        struct DSVClearParams
        {
            float clear_depth_value = 1.0f;
            uint8_t clear_stencil_value = 0;
        } dsv_clear_params;
    } clear_params;
    enum ClearType : uint8_t
    {
        NONE = 0,
        COLOR = BIT(1),
        DEPTH = BIT(2),
        STENCIL = BIT(3),
    } clear_type; // Can be combined
    Texture* texture;
};
} // namespace qhenki::gfx

#pragma once

#include <limits>

#include "enums.h"

namespace qhenki::gfx
{
struct SamplerDesc
{
    unsigned max_anisotropy = 0;
    float min_lod = 0.f;
    float max_lod = std::numeric_limits<float>::max();
    float mip_lod_bias = 0.f;
    float border_color[4]{0.f, 0.f, 0.f, 0.f};
    Filter min_filter = Filter::LINEAR;
    Filter mag_filter = Filter::LINEAR;
    Filter mip_filter = Filter::LINEAR;
    AddressMode address_mode_u = WRAP;
    AddressMode address_mode_v = WRAP;
    AddressMode address_mode_w = WRAP;
    bool comparison_enable = false;
    ComparisonFunc comparison_func = NEVER;
};
} // namespace qhenki::gfx

#pragma once

#include <array>
#include <cstdint>

#include "barrier.h"

namespace qhenki::gfx
{
enum class TextureDimension : uint8_t
{
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D
};

struct TextureDesc
{
    enum Usage : uint32_t
    {
        NONE = 0,
        COPY_SOURCE = BIT(0),
        COPY_DEST = BIT(1),
        SHADER_RESOURCE = BIT(2),
        UNORDERED_ACCESS = BIT(3),
        RENDER_TARGET = BIT(4),
        DEPTH_STENCIL = BIT(5),
        // BIT(6) reserved for possible transient attachment
        INPUT_ATTACHMENT = BIT(7),
        SHADING_RATE = BIT(8),
        VIDEO_DECODE = BIT(12),
        VIDEO_ENCODE = BIT(15),
    };

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t depth_or_array_size = 1;
    uint16_t mip_levels = 1;
    DXGI_FORMAT format; // TODO: replace type
    uint16_t sample_count = 1;
    TextureDimension dimension;
    bool is_cube = false;
    Layout initial_layout = Layout::COMMON;
    Usage usage = NONE;
    union
    {
        std::array<float, 4> clear_color_value;
        struct
        {
            float depth;
            uint8_t stencil;
        } clear_depth_value;
    };
};

using TextureUsage = TextureDesc::Usage;

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b)
{
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr TextureUsage& operator|=(TextureUsage& a, TextureUsage b)
{
    a = a | b;
    return a;
}
constexpr TextureUsage operator&(TextureUsage a, TextureUsage b)
{
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct Texture
{
    TextureDesc desc;
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx

#pragma once

#include <cstddef>
#include <cstdint>

namespace qhenki::gfx
{
enum ShaderType : uint8_t
{
    VERTEX_SHADER,
    PIXEL_SHADER,
    COMPUTE_SHADER,
    LIBRARY_SHADER,
};

enum class ShaderModel
{
    SM_5_0,
    SM_6_0,
    SM_6_1,
    SM_6_2,
    SM_6_3,
    SM_6_4,
    SM_6_5,
    SM_6_6,
    SM_6_7,
    SM_6_8,
    SM_6_9,
};

struct Shader
{
    void* data;
    size_t size;
};
} // namespace qhenki::gfx

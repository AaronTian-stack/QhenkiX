#pragma once

#include "qhenki/utility/math_util.h"

namespace qhenki::gfx
{
enum class IndexType : uint8_t
{
    UINT16,
    UINT32,
};

enum PrimitiveTopology : uint8_t
{
    POINT_LIST = 1,
    LINE_LIST = 2,
    LINE_STRIP = 3,
    TRIANGLE_LIST = 4,
    TRIANGLE_STRIP = 5,
};

enum class PipelineStage : uint8_t
{
    VERTEX,
    PIXEL,
    COMPUTE,
};

enum FillMode : uint8_t
{
    WIREFRAME = 2,
    SOLID = 3,
};

enum CullMode : uint8_t
{
    NONE = 1,
    FRONT = 2,
    BACK = 3,
};

enum StencilOp : uint8_t
{
    KEEP = 1,
    ZERO = 2,
    REPLACE = 3,
    INCREMENT_AND_CLAMP = 4,
    DECREMENT_AND_CLAMP = 5,
    INVERT = 6,
    INCREMENT = 7,
    DECREMENT = 8,
};

enum ComparisonFunc : uint8_t
{
    NEVER = 1,
    LESS = 2,
    EQUAL = 3,
    LESS_OR_EQUAL = 4,
    GREATER = 5,
    NOT_EQUAL = 6,
    GREATER_OR_EQUAL = 7,
    ALWAYS = 8,
};

enum class Blend : uint8_t
{
    ZERO,
    ONE,
    SRC_COLOR,
    INVERT_SRC_COLOR,
    SRC_ALPHA,
    INV_SRC_ALPHA,
    DEST_ALPHA,
    INVERT_DEST_ALPHA,
    DEST_COLOR,
    INVERT_DEST_COLOR,
    SRC_ALPHA_CLAMP,
    CONSTANT_COLOR,
    INVERT_CONSTANT_COLOR,
    SRC1_COLOR,
    INVERT_SRC1_COLOR,
    SRC1_ALPHA,
    INVERT_SRC1_ALPHA,
};

enum BlendOp : uint8_t
{
    ADD = 1,
    SUBTRACT = 2,
    REV_SUBTRACT = 3,
    MIN = 4,
    MAX = 5,
};

enum class LogicOp : uint8_t
{
    CLEAR,
    SET,
    COPY,
    COPY_INVERTED,
    NOOP,
    INVERT,
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    EQUIV,
    AND_REVERSE,
    AND_INVERTED,
    OR_REVERSE,
    OR_INVERTED,
};

enum class Format
{
    UNKNOWN,
    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R11G11B10_FLOAT,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,
    R16G16_FLOAT,
    R16G16_UNORM,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,
    D32_FLOAT,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,
    D24_UNORM_S8_UINT,
    R8G8_UNORM,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,
    R16_FLOAT,
    D16_UNORM,
    R16_UNORM,
    R16_UINT,
    R16_SNORM,
    R16_SINT,
    R8_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UF16,
    BC6H_SF16,
    BC7_UNORM,
    BC7_UNORM_SRGB,
};

enum SampleCount : uint8_t
{
    SAMPLE_COUNT_1 = BIT(0),
    SAMPLE_COUNT_2 = BIT(1),
    SAMPLE_COUNT_4 = BIT(2),
    SAMPLE_COUNT_8 = BIT(3),
    SAMPLE_COUNT_16 = BIT(4),
    SAMPLE_COUNT_32 = BIT(5),
    SAMPLE_COUNT_64 = BIT(6),
};

enum Filter : uint8_t
{
    NEAREST = 0,
    LINEAR = 1,
};

enum AddressMode : uint8_t
{
    WRAP = 1,
    MIRROR = 2,
    CLAMP = 3,
    BORDER = 4,
};
} // namespace qhenki::gfx

#pragma once

#include <array>
#include <optional>

#include "enums.h"

namespace qhenki::gfx
{
struct RasterizerDesc
{
    FillMode fill_mode = SOLID;
    CullMode cull_mode = NONE;
    int depth_bias = 0;
    float depth_bias_clamp = 0.0f;
    float slope_scaled_depth_bias = 0.0f;
    bool front_counter_clockwise = true;
    bool depth_clip_enable = true;
    // Always uses alpha MSAA
    // No AA lines
    // TODO: Conservative Rasterization?
};

struct DepthStencilOpDesc
{
    StencilOp fail_op;
    StencilOp depth_fail_op;
    StencilOp pass_op;
    ComparisonFunc func;
};

struct DepthStencilDesc
{
    DepthStencilOpDesc front_face;
    DepthStencilOpDesc back_face;
    bool depth_write_enable = true;
    ComparisonFunc depth_func = LESS;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    bool depth_enable = true;
    bool stencil_enable = false;
};

constexpr unsigned MAX_RENDER_TARGETS = 8u;

struct BlendDesc
{
    bool alpha_to_coverage_enable;
    bool independent_blend_enable;
    struct RenderTargetBlendDesc
    {
        bool blend_enable;
        bool logic_op_enable;
        Blend src_blend;
        Blend dst_blend;
        BlendOp blend_op;
        Blend src_blend_alpha;
        Blend dst_blend_alpha;
        BlendOp blend_op_alpha;
        LogicOp logic_op;
        uint8_t render_target_write_mask;
    };
    RenderTargetBlendDesc render_target[MAX_RENDER_TARGETS];
};

struct GraphicsPipelineDesc
{
    std::optional<BlendDesc> blend_desc;
    std::optional<DepthStencilDesc> depth_stencil_state;
    std::array<Format, MAX_RENDER_TARGETS> rtv_formats{};
    std::optional<RasterizerDesc> rasterizer_state;
    SampleCount sample_count = SAMPLE_COUNT_1;
    unsigned num_render_targets = 0;
    Format dsv_format{};
    PrimitiveTopology topology = TRIANGLE_LIST;
    bool increment_slot = false; // Whether to increment slot of input, used during reflection
};

struct GraphicsPipeline
{
    sPtr<void> internal_state;
};

struct LayoutBinding
{
    uint32_t binding;
    uint32_t count;
    enum RangeType : uint8_t
    {
        SRV = 0,
        UAV = SRV + 1,
        CBV = UAV + 1,
        SAMPLER = CBV + 1,
    } type;
};

struct PushRange
{
    uint32_t size;
    uint32_t binding;
    uint32_t space;
};

constexpr auto INFINITE_DESCRIPTORS = 0xFFFFFFFF;
constexpr size_t MAX_SPACES = 7; // Defined based off Vulkan max descriptor sets

struct PipelineLayoutDesc
{
    std::vector<PushRange> push_ranges;                        // TODO: use small vector
    std::array<std::vector<LayoutBinding>, MAX_SPACES> spaces; // TODO: use small vector
};

struct PipelineLayout
{
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx

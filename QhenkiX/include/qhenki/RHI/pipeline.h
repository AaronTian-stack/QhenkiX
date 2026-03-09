#pragma once

#include <directx/d3d12.h>
#include <directx/dxgiformat.h>

#include <array>
#include <cstdint>
#include <optional>

#include "enums.h"

namespace qhenki::gfx
{
// TODO: Replace D3D types
struct RasterizerDesc
{
    D3D12_FILL_MODE fill_mode = D3D12_FILL_MODE_SOLID;
    D3D12_CULL_MODE cull_mode = D3D12_CULL_MODE_NONE;
    int depth_bias = 0;
    float depth_bias_clamp = 0.0f;
    float slope_scaled_depth_bias = 0.0f;
    bool front_counter_clockwise = true;
    bool depth_clip_enable = true;
    // Always uses alpha MSAA
    // No AA lines
    // TODO: Conservative Rasterization?
};

struct DepthStencilDesc
{
    D3D12_DEPTH_STENCILOP_DESC front_face;
    D3D12_DEPTH_STENCILOP_DESC back_face;
    D3D12_DEPTH_WRITE_MASK depth_write_mask = D3D12_DEPTH_WRITE_MASK_ALL;
    D3D12_COMPARISON_FUNC depth_func = D3D12_COMPARISON_FUNC_LESS;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    bool depth_enable = true;
    bool stencil_enable = false;
    // Vulkan uses a single struct for both front and back face (read/write mask)
};

struct InputLayoutDesc
{
    uint32_t num_elements;
    D3D12_INPUT_ELEMENT_DESC* elements;
};

constexpr unsigned MAX_RENDER_TARGETS = 8u;

struct GraphicsPipelineDesc
{
    std::optional<D3D12_BLEND_DESC> blend_desc;
    std::optional<DepthStencilDesc> depth_stencil_state;
    std::array<DXGI_FORMAT, MAX_RENDER_TARGETS> rtv_formats{};
    std::optional<RasterizerDesc> rasterizer_state;
    std::optional<InputLayoutDesc> input_layout;
    unsigned sample_count = 1;
    unsigned num_render_targets = 0;
    DXGI_FORMAT dsv_format{};
    PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
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
    enum class RangeType : uint8_t
    {
        SRV_BUFFER,
        SRV_TEXTURE,
        UAV_BUFFER,
        UAV_TEXTURE,
        CBV,
        SAMPLER,
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

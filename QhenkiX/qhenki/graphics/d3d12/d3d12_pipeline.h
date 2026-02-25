#pragma once

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D12Pipeline
{
    D3D12_INPUT_ELEMENT_DESC* input_layout_desc = nullptr;
    ComPtr<ID3D12PipelineState> pipeline_state{};
    D3D12_PRIMITIVE_TOPOLOGY primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; // Needed for command list
};
} // namespace qhenki::gfx

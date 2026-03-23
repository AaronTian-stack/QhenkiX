#pragma once

#include <directx/d3d12.h>
#include <directx/d3dcommon.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D12Pipeline
{
    ComPtr<ID3D12PipelineState> pipeline_state{};
    D3D12_PRIMITIVE_TOPOLOGY primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; // Needed for command list
};
} // namespace qhenki::gfx

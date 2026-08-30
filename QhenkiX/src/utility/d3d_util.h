#pragma once

#include <directx/d3d12.h>
#include <directx/dxgiformat.h>
#include <wrl/client.h>
#include <string>

#include "qhenki/rhi/barrier.h"
#include "qhenki/rhi/enums.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
DXGI_FORMAT dxgi_format(IndexType format);

D3D12_BARRIER_SYNC sync_stage(SyncStage stage);
D3D12_BARRIER_ACCESS access_flags(AccessFlags access);
D3D12_BARRIER_LAYOUT layout(Layout layout);

// Only returns the filter types shared with D3D11
D3D12_FILTER filter(Filter min, Filter mag, Filter mip, bool comparison_enable, UINT max_anisotropy);

// Note blend is subset of D3D12_BLEND (D3D11_BLEND) otherwise they are equal
D3D12_BLEND blend(Blend blend);

// D3D12_LOGIC_OP = D3D11_LOGIC_OP
D3D12_LOGIC_OP logic_op(LogicOp logic_op);
} // namespace qhenki::gfx

#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D12CommandList
{
    // Current root signature
    ID3D12RootSignature* root_signature = nullptr;
    ComPtr<ID3D12GraphicsCommandList7> list;
};
} // namespace qhenki::gfx

#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D11_SRV_UAV_Heap
{
    std::vector<ComPtr<ID3D11ShaderResourceView>> shader_resource_views;
    std::vector<ComPtr<ID3D11UnorderedAccessView>> unordered_access_views;
};
typedef std::vector<ComPtr<ID3D11SamplerState>> D3D11_Sampler_Heap;
} // namespace qhenki::gfx

#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <variant>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
struct D3D11Texture
{
    std::variant<ComPtr<ID3D11Texture1D>, ComPtr<ID3D11Texture2D>, ComPtr<ID3D11Texture3D>> texture;
    ComPtr<ID3D11RenderTargetView> rtv_view;
    ComPtr<ID3D11DepthStencilView> dsv_view;
};
} // namespace qhenki::gfx

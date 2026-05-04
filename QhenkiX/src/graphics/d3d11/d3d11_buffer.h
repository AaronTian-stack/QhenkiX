#pragma once

#include <d3d11.h>

namespace qhenki::gfx
{
struct D3D11Buffer
{
    ComPtr<ID3D11Buffer> buffer;
    D3D11_USAGE mapping_type;
};
} // namespace qhenki::gfx

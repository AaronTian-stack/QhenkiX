#pragma once

#include <cstdint>

#include <directx/d3dcommon.h>
#include <directx/dxgiformat.h>

namespace qhenki::gfx
{
DXGI_FORMAT mask_to_format(uint32_t mask, D3D_REGISTER_COMPONENT_TYPE type);
} // namespace qhenki::gfx

#pragma once

#include <directx/dxgiformat.h>

#include "qhenki/RHI/enums.h"

namespace qhenki::gfx
{
DXGI_FORMAT dxgi_format(Format format);
// Inverse of dxgi_format(Format)
Format format_from_dxgi(DXGI_FORMAT format);
} // namespace qhenki::gfx

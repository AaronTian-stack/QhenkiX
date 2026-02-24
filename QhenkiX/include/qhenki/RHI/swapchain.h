#pragma once

#include <directx/dxgiformat.h>

namespace qhenki::gfx
{
struct SwapchainDesc
{
    unsigned width;
    unsigned height;
    DXGI_FORMAT format;
    unsigned buffer_count;
    bool tearing;
};
typedef SwapchainDesc Swapchain;
} // namespace qhenki::gfx

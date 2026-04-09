#pragma once

#include <directx/dxgiformat.h>

namespace qhenki::gfx
{
// Swapchains are managed internally within the context, so there is no data held here.
// You should not interact with this struct directly after initialization.
struct SwapchainDesc
{
    unsigned width;
    unsigned height;
    Format format;
    unsigned buffer_count;
    bool tearing;
};
typedef SwapchainDesc Swapchain;
} // namespace qhenki::gfx

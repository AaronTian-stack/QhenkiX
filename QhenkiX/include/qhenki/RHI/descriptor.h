#pragma once

#include "descriptor_heap.h"

namespace qhenki::gfx
{
struct Descriptor
{
    DescriptorHeap* heap = nullptr;
    // Offset into heap in descriptors, or offset into list of views for compatibility mode
    size_t offset = 0;
};
} // namespace qhenki::gfx

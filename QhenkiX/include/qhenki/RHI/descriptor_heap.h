#pragma once

#include "smartpointer.h"

namespace qhenki::gfx
{
struct DescriptorHeapDesc
{
    enum class Type
    {
        CBV_SRV_UAV,
        SAMPLER,
    } type;
    enum class Visibility
    {
        CPU,
        GPU,
    } visibility;
    // Size in bytes. Need to query to know size for N descriptors. Note that because of alignment, the size could be
    // different depending on the order you want to place the descriptors in if you mix buffer and texture descriptors,
    // so you may need a conservative size
    size_t size;
    // Whether to bloat buffer/texture descriptors. This only affects logic during creation of descriptors / placement
    // within heap. You should only use this when doing full heap indexing in shaders. Note that you need to account for
    // bloated size yourself when this is true i.e size = N * max(buffer_descriptor_size, texture_descriptor_size)
    bool bloat_descriptors;
};

struct DescriptorHeap
{
    DescriptorHeapDesc desc;
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx

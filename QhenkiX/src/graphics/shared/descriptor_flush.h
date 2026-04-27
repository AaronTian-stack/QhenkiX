#pragma once

#include <span>
#include <vector>

#include "qhenki/RHI/descriptor.h"

namespace qhenki::gfx
{
struct MinimalDescriptor
{
    DescriptorHeap* heap;
    size_t offset;
};

struct PendingDescriptorCopy
{
    MinimalDescriptor src;
    MinimalDescriptor dst;
    size_t descriptors;
};

class DeferredDescriptorCopier
{
    std::vector<PendingDescriptorCopy> pending_copies;
    std::vector<PendingDescriptorCopy> merged_copies;

public:
    bool add_pending_descriptor_copy(size_t descriptors, const Descriptor& src, const Descriptor& dst);
    size_t merge_regions();
    std::span<const PendingDescriptorCopy> get_merged_regions() const;
    void reset();
};

}; // namespace qhenki::gfx

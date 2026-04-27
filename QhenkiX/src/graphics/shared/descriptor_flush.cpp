#include "descriptor_flush.h"

#include <algorithm>
#include <vector>

using namespace qhenki::gfx;

namespace
{
bool pending_copy_less(const PendingDescriptorCopy& a, const PendingDescriptorCopy& b)
{
    if (a.src.heap != b.src.heap)
    {
        return a.src.heap < b.src.heap;
    }
    if (a.src.offset != b.src.offset)
    {
        return a.src.offset < b.src.offset;
    }
    if (a.dst.heap != b.dst.heap)
    {
        return a.dst.heap < b.dst.heap;
    }
    return a.dst.offset < b.dst.offset;
}

bool can_merge_adjacent(const PendingDescriptorCopy& cur, const PendingDescriptorCopy& next)
{
    if (cur.src.heap != next.src.heap || cur.dst.heap != next.dst.heap)
    {
        return false;
    }
    if (cur.src.offset + cur.descriptors != next.src.offset)
    {
        return false;
    }
    if (cur.dst.offset + cur.descriptors != next.dst.offset)
    {
        return false;
    }
    return true;
}
} // namespace

bool DeferredDescriptorCopier::add_pending_descriptor_copy(const size_t descriptors,
                                                           const Descriptor& src,
                                                           const Descriptor& dst)
{
    if (src.heap->desc.type != dst.heap->desc.type)
    {
        return false;
    }
    pending_copies.push_back({.src = {.heap = src.heap, .offset = src.offset},
                              .dst = {.heap = dst.heap, .offset = dst.offset},
                              .descriptors = descriptors});
    return true;
}

size_t DeferredDescriptorCopier::merge_regions()
{
    merged_copies.clear();

    if (pending_copies.empty())
    {
        return 0;
    }

    if (pending_copies.size() > 1)
    {
        std::ranges::sort(pending_copies, pending_copy_less);
    }

    merged_copies.reserve(pending_copies.size());
    merged_copies.push_back(pending_copies.front());

    for (size_t i = 1; i < pending_copies.size(); ++i)
    {
        PendingDescriptorCopy& cur = merged_copies.back();
        const PendingDescriptorCopy& next = pending_copies[i];

        if (can_merge_adjacent(cur, next))
        {
            cur.descriptors += next.descriptors;
        }
        else
        {
            merged_copies.push_back(next);
        }
    }

    return pending_copies.size() - merged_copies.size();
}

std::span<const PendingDescriptorCopy> DeferredDescriptorCopier::get_merged_regions() const
{
    return merged_copies;
}

void DeferredDescriptorCopier::reset()
{
    pending_copies.clear();
    merged_copies.clear();
}

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
    if (cur.src.offset + cur.bytes != next.src.offset)
    {
        return false;
    }
    if (cur.dst.offset + cur.bytes != next.dst.offset)
    {
        return false;
    }
    return true;
}
} // namespace

void DeferredDescriptorCopier::add_pending_descriptor_copy(const size_t bytes,
                                                           const Descriptor& src,
                                                           const Descriptor& dst)
{
    pending_copies.push_back({.src = {.heap = src.heap, .offset = src.offset},
                              .dst = {.heap = dst.heap, .offset = dst.offset},
                              .bytes = bytes});
}

size_t DeferredDescriptorCopier::get_max_segments() const
{
    return pending_copies.size();
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
            cur.bytes += next.bytes;
        }
        else
        {
            merged_copies.push_back(next);
        }
    }

    return pending_copies.size() - merged_copies.size();
}

const PendingDescriptorCopy* DeferredDescriptorCopier::get_merged_regions(size_t* count) const
{
    *count = merged_copies.size();
    return merged_copies.data();
}

void DeferredDescriptorCopier::reset()
{
    pending_copies.clear();
    merged_copies.clear();
}

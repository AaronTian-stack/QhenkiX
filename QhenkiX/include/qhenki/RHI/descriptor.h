#pragma once

class VmaVirtualAllocation_T;
typedef VmaVirtualAllocation_T* VmaVirtualAllocation;

#include <vk_mem_alloc.h>

#include <limits>
#include "descriptor_heap.h"

namespace qhenki::gfx
{
// Creates a new descriptor in the heap, otherwise use the already existing offset to recreate the descriptor
constexpr size_t CREATE_NEW_DESCRIPTOR = std::numeric_limits<size_t>::max();

struct Descriptor
{
    DescriptorHeap* heap = nullptr;
    // Offset into heap in bytes, or offset into list of views for compatibility mode
    size_t offset = CREATE_NEW_DESCRIPTOR;
    enum Type : uint8_t
    {
        BUFFER,
        TEXTURE,
        SAMPLER,
    };

    Descriptor() = default;

    Descriptor(DescriptorHeap* heap, const size_t offset)
        : heap(heap),
          offset(offset)
    {
    }

private:
    // Only used in Vulkan backend
    VmaVirtualAllocation alloc = nullptr;
    friend class VulkanContext;
    friend class VulkanDescriptorHeap;
};
} // namespace qhenki::gfx

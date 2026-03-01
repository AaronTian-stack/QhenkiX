#pragma once

#include "qhenki/RHI/descriptor_heap.h"

#include "vulkan_buffer.h"
#include "vulkan_context.h"

namespace qhenki::gfx
{
class VulkanDescriptorHeap
{
    VulkanBuffer m_heap;
    VmaVirtualBlock m_block;

public:
    bool create(const DescriptorHeapDesc& desc, const VulkanContext& context);
    bool allocate(VmaVirtualAllocation* va);
    bool deallocate();
};
} // namespace qhenki::gfx

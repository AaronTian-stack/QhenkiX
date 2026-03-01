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

    VulkanContext* m_context = nullptr;

public:
    VulkanDescriptorHeap() = default;
    ~VulkanDescriptorHeap();
    bool create(const DescriptorHeapDesc& desc, const VulkanContext& context);
    bool allocate(VmaVirtualAllocation* va,
                  Descriptor::Type type,
                  const VulkanContext& context,
                  VkDeviceSize* offset) const;
    void deallocate(VmaVirtualAllocation va) const;
};
} // namespace qhenki::gfx

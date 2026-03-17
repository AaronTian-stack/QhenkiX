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

    const VulkanContext* m_context = nullptr;
    VkDeviceSize m_reserved_size = 0;
    void* m_data = nullptr; // Persistently mapped pointer to write into CPU descriptor heaps only

public:
    VulkanDescriptorHeap() = default;
    ~VulkanDescriptorHeap();
    bool create(const DescriptorHeapDesc& desc, const VulkanContext& context);
    bool allocate(VmaVirtualAllocation* va, Descriptor::Type type, VkDeviceSize* offset) const;
    void deallocate(VmaVirtualAllocation va) const;
    void* get_cpu_pointer(size_t offset) const;
    VkDeviceAddress get_gpu_address() const;
    VkDeviceSize get_reserved_size() const;
    VmaVirtualAllocationInfo get_allocation_info(VmaVirtualAllocation va) const;
};
} // namespace qhenki::gfx

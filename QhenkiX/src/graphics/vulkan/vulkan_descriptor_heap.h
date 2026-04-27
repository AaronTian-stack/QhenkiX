#pragma once

#include "qhenki/RHI/descriptor_heap.h"

#include "vulkan_buffer.h"

namespace qhenki::gfx
{
struct VulkanDescriptorHeapInitInfo
{
    VmaAllocator allocator;
    struct DescriptorHeapInfo
    {
        VkDeviceSize reserved_size;
        VkDeviceSize max_size;
        VkDeviceSize heap_alignment;
        VkDeviceSize descriptor_size;
    } heap_info;
};

class VulkanDescriptorHeap
{
    VulkanBuffer m_heap;

    VmaAllocator m_allocator = nullptr;

    VkDeviceSize m_total_size = 0;
    VkDeviceSize m_reserved_size = 0;
    void* m_data = nullptr; // Persistently mapped pointer to write into CPU descriptor heaps only

public:
    VulkanDescriptorHeap() = default;
    VulkanDescriptorHeap(const VulkanDescriptorHeap&) = delete;
    VulkanDescriptorHeap(VulkanDescriptorHeap&&) = delete;
    VulkanDescriptorHeap& operator=(const VulkanDescriptorHeap&) = delete;
    VulkanDescriptorHeap& operator=(VulkanDescriptorHeap&&) = delete;
    ~VulkanDescriptorHeap();

    bool create(const DescriptorHeapDesc& desc, const VulkanDescriptorHeapInitInfo& init_info);

    void* get_cpu_pointer(size_t offset) const;
    VkBuffer get_buffer() const;
    VkDeviceSize get_total_size() const;
};
} // namespace qhenki::gfx

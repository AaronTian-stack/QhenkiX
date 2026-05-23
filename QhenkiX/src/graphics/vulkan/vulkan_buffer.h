#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocator allocator;
    // We need default constructor for VulkanDescriptorHeap member
    VulkanBuffer() = default;
    VulkanBuffer(const VkBuffer buffer, const VmaAllocation allocation, const VmaAllocator allocator)
        : buffer(buffer),
          allocation(allocation),
          allocator(allocator)
    {
    }
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(VulkanBuffer&&) = delete;
    ~VulkanBuffer()
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
};
} // namespace qhenki::gfx

#pragma once

#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocator allocator;
    // All types are pointers
    VulkanBuffer() = default;
    VulkanBuffer(const VulkanBuffer&) = default;
    VulkanBuffer(VulkanBuffer&&) = default;
    VulkanBuffer& operator=(const VulkanBuffer&) = default;
    VulkanBuffer& operator=(VulkanBuffer&&) = default;
    ~VulkanBuffer()
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
};
} // namespace qhenki::gfx

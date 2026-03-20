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

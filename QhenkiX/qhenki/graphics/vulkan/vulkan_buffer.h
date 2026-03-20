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
    VkDeviceAddress get_gpu_address(const VkDevice device) const
    {
        const VkBufferDeviceAddressInfo addr_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = buffer,
        };
        return vkGetBufferDeviceAddress(device, &addr_info);
    }
};
} // namespace qhenki::gfx

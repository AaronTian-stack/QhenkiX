#pragma once

#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanTexture
{
    VkImage image;
    VmaAllocation allocation;
    VmaAllocator allocator;
    ~VulkanTexture()
    {
        vmaDestroyImage(allocator, image, allocation);
    }
};
} // namespace qhenki::gfx

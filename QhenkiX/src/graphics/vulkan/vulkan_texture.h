#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanTexture
{
    VkImage image;
    VmaAllocation allocation;
    VmaAllocator allocator;
    VkImageLayout initial_layout;
    ~VulkanTexture()
    {
        vmaDestroyImage(allocator, image, allocation);
    }
};
} // namespace qhenki::gfx

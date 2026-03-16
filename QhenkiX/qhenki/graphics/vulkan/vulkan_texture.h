#pragma once

#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanTexture
{
    VkImage image;
    VmaAllocation allocation;
    VmaAllocator allocator;
    bool has_transitioned = false;
    VkImageLayout initial_layout;
    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    ~VulkanTexture()
    {
        vmaDestroyImage(allocator, image, allocation);
    }
};
} // namespace qhenki::gfx

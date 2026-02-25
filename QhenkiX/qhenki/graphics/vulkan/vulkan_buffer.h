#pragma once

#include <vk_mem_alloc.h>

namespace qhenki::gfx
{
struct VulkanBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};
} // namespace qhenki::gfx

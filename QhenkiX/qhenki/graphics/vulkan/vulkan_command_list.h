#pragma once

#include <vulkan/vulkan.h>

namespace qhenki::gfx
{
struct VulkanCommandList
{
    VkCommandBuffer cmd_buf;
    uint32_t push_range_count;
};
} // namespace qhenki::gfx

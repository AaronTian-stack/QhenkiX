#pragma once

#include <vulkan/vulkan.h>

namespace qhenki::gfx
{
struct VulkanCommandList
{
    VkCommandBuffer cmd_buf;
    VulkanRootSignature* root_signature;
};
} // namespace qhenki::gfx

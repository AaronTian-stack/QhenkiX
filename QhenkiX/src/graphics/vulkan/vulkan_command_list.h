#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <array>

namespace qhenki::gfx
{
struct VulkanCommandList
{
    VkCommandBuffer cmd_buf;
    VulkanRootSignature* root_signature;
    std::array<char, 64> debug_name;
};
} // namespace qhenki::gfx

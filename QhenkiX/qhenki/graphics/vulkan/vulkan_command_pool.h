#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace qhenki::gfx
{
class VulkanCommandPool
{
    VkDevice device;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

public:
    // Expects pool to already be created
    explicit VulkanCommandPool(VkDevice device, VkCommandPool pool);
    VulkanCommandPool(const VulkanCommandPool&) = default;
    VulkanCommandPool& operator=(const VulkanCommandPool&) = default;
    VulkanCommandPool(VulkanCommandPool&&) = default;
    ~VulkanCommandPool();

    VkCommandBuffer create_command_buffer(VkCommandBufferAllocateInfo& info);
    VkResult reset();
};
} // namespace qhenki::gfx

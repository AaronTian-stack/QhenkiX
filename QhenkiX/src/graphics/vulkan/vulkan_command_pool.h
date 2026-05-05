#pragma once

#include <volk/volk.h>

#include <array>

namespace qhenki::gfx
{
class VulkanCommandPool
{
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    // You shouldn't be submitting so many command buffers
    std::array<VkCommandBuffer, 32> command_buffers{};
    unsigned command_buffer_count = 0;

public:
    VulkanCommandPool() = default;

    // Expects pool to already be created
    VulkanCommandPool(VkDevice device, VkCommandPool pool);
    VulkanCommandPool(const VulkanCommandPool&) = default;
    VulkanCommandPool& operator=(const VulkanCommandPool&) = default;
    VulkanCommandPool(VulkanCommandPool&&) = default;
    ~VulkanCommandPool();

    void init(VkDevice device, VkCommandPool pool);

    VkCommandBuffer create_command_buffer(VkCommandBufferAllocateInfo& info);
    VkResult reset();

    friend class VulkanContext;
};
} // namespace qhenki::gfx
